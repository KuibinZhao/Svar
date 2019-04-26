#define __SVAR_BUILDIN__
#include "Svar.h"
#include "gtest.h"
#include "SharedLibrary.h"

using namespace GSLAM;

TEST(Svar,Variable)
{
    EXPECT_TRUE(Svar()==Svar::Undefined());
    EXPECT_TRUE(Svar(nullptr)==Svar::Null());

    Svar var(false);
    EXPECT_EQ(var.typeName(),"bool");
    EXPECT_TRUE(var.is<bool>());
    EXPECT_FALSE(var.as<bool>());

    EXPECT_TRUE(Svar(1).is<int>());
    EXPECT_TRUE(Svar(1).as<int>()==1);

    EXPECT_TRUE(Svar("").as<std::string>().empty());
    EXPECT_TRUE(Svar(1.).as<double>()==1.);
    EXPECT_TRUE(Svar(std::vector<int>({1,2})).isArray());
    EXPECT_TRUE(Svar(std::map<int,Svar>({{1,2}})).isDict());

    Svar obj(std::map<std::string,int>({{"1",1}}));
    EXPECT_TRUE(obj.isObject());
    EXPECT_TRUE(obj["1"]==1);

}

std::string add(std::string left,const std::string& r){
    return left+r;
}

TEST(Svar,Function)
{
    int intV=0,srcV=0;
    Svar intSvar(0);
    // Svar argument is recommended
    SvarFunction isBool([](Svar config){return config.is<bool>();});
    EXPECT_TRUE(isBool.call(false).as<bool>());
    // pointer
    Svar::lambda([](int* ref){*ref=10;})(&intV);
    Svar::lambda([](int* ref){*ref=10;})(intSvar);
    EXPECT_EQ(intV,10);
    EXPECT_EQ(intSvar,10);
    // WARNNING!! ref, should pay attention that when using ref it should always input the svar instead of the raw ref
    Svar::lambda([](int& ref){ref=20;})(intSvar);
    EXPECT_EQ(intSvar,20);
    // const ref
    Svar::lambda([](const int& ref,int* dst){*dst=ref;})(30,&intV);
    EXPECT_EQ(intV,30);
    // const pointer
    Svar::lambda([](const int* ref,int* dst){*dst=*ref;})(&srcV,&intV);
    EXPECT_EQ(intV,srcV);
    // overload is only called when cast is not available!
    Svar funcReturn=Svar::lambda([](int i){return i;});
    funcReturn.as<SvarFunction>().next=Svar::lambda([](char c){return c;});
    EXPECT_EQ(funcReturn(1).cpptype(),typeid(int));
    EXPECT_EQ(funcReturn('c').cpptype(),typeid(char));
    // string argument
    Svar s("hello");
    Svar::lambda([](std::string raw,
                 const std::string c,
                 const std::string& cref,
                 std::string& ref,
                 std::string* ptr,
                 const std::string* cptr){
        EXPECT_EQ(raw,"hello");
        EXPECT_EQ(c,"hello");
        EXPECT_EQ(ref,"hello");
        EXPECT_EQ(cref,"hello");
        EXPECT_EQ((*ptr),"hello");
        EXPECT_EQ((*cptr),"hello");
    })(s,s,s,s,s,s);
    // cpp function binding
    EXPECT_EQ(Svar(add)("a",std::string("b")).as<std::string>(),"ab");
    // static method function binding
    EXPECT_TRUE(Svar::Null().isNull());
    EXPECT_TRUE(Svar(&Svar::Null)().isNull());

}

class BaseClass{
public:
    BaseClass():age_(0){}
    BaseClass(int age):age_(age){}

    int getAge()const{return age_;}
    void setAge(int a){age_=a;}

    static BaseClass create(int a){return BaseClass(a);}
    static BaseClass create1(){return BaseClass();}

    virtual std::string intro()const{return "age:"+std::to_string(age_);}

    int age_;
};

class InheritClass: public BaseClass
{
public:
    InheritClass(int age,std::string name):BaseClass(age),name_(name){}

    std::string name()const{return name_;}
    std::string setName(std::string name){name_=name;}

    virtual std::string intro() const{return BaseClass::intro()+", name:"+name_;}
    std::string name_;
};


TEST(Svar,Class){
    SvarClass::Class<BaseClass>()
            .def_static("__init__",[](){return BaseClass();})
            .def_static("__init__",[](int age){return BaseClass(age);})
            .def("getAge",&BaseClass::getAge)
            .def("setAge",&BaseClass::setAge)
            .def_static("create",&BaseClass::create)
            .def_static("create1",&BaseClass::create1)
            .def("intro",&BaseClass::intro);
    Svar a=Svar::create(BaseClass(10));
    Svar baseClass=a.classObject();
    a=baseClass["__init__"](10);
    a=baseClass();//
    a=baseClass(10);//
    EXPECT_EQ(a.call("getAge").as<int>(),10);
    EXPECT_EQ(a.call("intro").as<std::string>(),BaseClass(10).intro());

    SvarClass::Class<InheritClass>()
            .inherit<BaseClass>()
            .def("__init__",[](int age,std::string name){return InheritClass(age,name);})
            .def("name",&InheritClass::name)
            .def("intro",&InheritClass::intro);
    Svar b=Svar::create(InheritClass(10,"xm"));
    EXPECT_EQ(b.call("getAge").as<int>(),10);
    EXPECT_EQ(b.call("name").as<std::string>(),"xm");
    EXPECT_EQ(b.call("intro").as<std::string>(),InheritClass(10,"xm").intro());
    b.call("setAge",20);
    EXPECT_EQ(b.call("getAge").as<int>(),20);
}

TEST(Svar,GetSet){
    Svar var;
    int& testInt=var.GetInt("child.testInt",20);
    EXPECT_EQ(testInt,20);
    testInt=30;
    EXPECT_EQ(var.GetInt("child.testInt"),30);
    var.set("child.testInt",40);
    EXPECT_EQ(testInt,40);
    EXPECT_EQ(var["child"]["testInt"],40);
    var.set("int",100);
    var.set("double",100);
    var.set<std::string>("string","100");
    var.set("bool",true);
    EXPECT_EQ(var["int"],100);
    EXPECT_EQ(var["double"],100);
    EXPECT_EQ(var["string"],100);
    EXPECT_EQ(var["bool"],true);
}

TEST(Svar,Call){
    EXPECT_EQ(Svar(1).call("__str__"),"1");// Call as member methods
    EXPECT_EQ(SvarClass::instance<int>().call("__str__",1),"1");// Call as static function
}

TEST(Svar,Cast){
    EXPECT_EQ(Svar(2.).castAs<int>(),2);
    EXPECT_TRUE(Svar(1).castAs<double>()==1.);
}

TEST(Svar,NumOp){
    EXPECT_EQ(-Svar(2.1),-2.1);
    EXPECT_EQ(-Svar(2),-2);
    EXPECT_EQ(Svar(2.1)+Svar(1),3.1);
    EXPECT_EQ(Svar(4.1)-Svar(2),4.1-2);
    EXPECT_EQ(Svar(3)*Svar(3.3),3*3.3);
    EXPECT_EQ(Svar(5.4)/Svar(2),5.4/2);
    EXPECT_EQ(Svar(5)%Svar(2),5%2);
    EXPECT_EQ(Svar(5)^Svar(2),5^2);
    EXPECT_EQ(Svar(5)|Svar(2),5|2);
    EXPECT_EQ(Svar(5)&Svar(2),5&2);
}

TEST(Svar,Dump){
    return;
    Svar obj(std::map<std::string,int>({{"1",1}}));
    std::cout<<obj<<std::endl;
    std::cout<<Svar::array({1,2,3})<<std::endl;
    std::cout<<Svar::create((std::type_index)typeid(1))<<std::endl;
    std::cout<<Svar::lambda([](std::string sig){})<<std::endl;
    std::cout<<SvarClass::Class<int>();
    std::cout<<Svar::instance();
}

TEST(Svar,Iterator){
    auto vec=Svar::array({1,2,3});
    Svar arrayiter=vec.call("__iter__");
    Svar next=arrayiter.classObject()["next"];
    int i=0;
    for(Svar it=next(arrayiter);!it.isUndefined();it=next(arrayiter)){
        EXPECT_EQ(vec[i++],it);
    }
    i=0;
    for(Svar it=arrayiter.call("next");!it.isUndefined();it=arrayiter.call("next"))
    {
        EXPECT_EQ(vec[i++],it);
    }
}

TEST(Svar,Json){
    Svar var;
    var.set("i",1);
    var.set("d",2.);
    var.set("s",Svar("str"));
    var.set("b",false);
    var.set("l",Svar::array({1,2,3}));
    var.set("m",Svar::object({{"a",1},{"b",false}}));
    std::string str=Json::dump(var);
    Svar varCopy=Json::load(str);
    EXPECT_EQ(str,Json::dump(varCopy));
}

TEST(Svar,Thread){
    auto readThread=[](){
        while(!svar.get<bool>("shouldstop",false)){
            double& d=svar.get<double>("thread.Double",100);
            usleep(1);
        }
    };
    auto writeThread=[](){
        svar.set<double>("thread.Double",svar.GetDouble("thread.Double")++);
        while(!svar.get<bool>("shouldstop",false)){
            svar.set<int>("thread.Double",10);// use int instead of double to violately testing robustness
            usleep(1);
        }
    };
    std::vector<std::thread> threads;
    for(int i=0;i<svar.GetInt("readThreads",4);i++) threads.push_back(std::thread(readThread));
    for(int i=0;i<svar.GetInt("writeThreads",4);i++) threads.push_back(std::thread(writeThread));
    sleep(1);
    svar.set("shouldstop",true);
    for(auto& it:threads) it.join();
}

void showPlugin(){
    std::string pluginPath=svar.GetString("plugin");
    std::string key=svar.GetString("key");

    SharedLibraryPtr plugin(new SharedLibrary(pluginPath));
    if(!plugin->isLoaded()){
        std::cerr<<"Unable to load plugin "<<pluginPath<<std::endl;
        return ;
    }

    GSLAM::Svar* (*getInst)()=(GSLAM::Svar* (*)())plugin->getSymbol("svarInstance");
    if(!getInst){
        std::cerr<<"No svarInstance found in "<<pluginPath<<std::endl;
        return ;
    }
    GSLAM::Svar* inst=getInst();
    if(!inst){
        std::cerr<<"svarInstance returned null.\n";
        return ;
    }

    GSLAM::Svar var=key.empty()?(*inst):inst->get(key,Svar());

    if(var.isFunction())
        std::cout<<var.as<SvarFunction>()<<std::endl;
    else if(var.isClass())
        std::cout<<var.as<SvarClass>()<<std::endl;
    else std::cout<<var;
}

int main(int argc,char** argv){
    Svar var=svar;
    Svar unParsed=var.parseMain(argc,argv);

    std::string plugin=var.arg<std::string>("plugin","","The svar module plugin wanna to show.");
    var.arg<std::string>("key","","The name in the module wanna to show");
    bool tests=var.arg<bool>("tests",false,"Perform google tests.");
    bool dumpVar=var.arg<bool>("dumpVar",false,"DumpVar");

    if(var.get<bool>("help",false)||argc<2){
        var.help();
        return 0;
    }

    if(unParsed.length()){
        std::cerr<<"Unparsed arguments:"<<unParsed<<std::endl;
        return -1;
    }

    if(plugin.size()){
        showPlugin();
        return 0;
    }

    if(tests){
        testing::InitGoogleTest(&argc,argv);
        return RUN_ALL_TESTS();
    }

    if(dumpVar){
        std::cout<<var;
    }

    return 0;
}