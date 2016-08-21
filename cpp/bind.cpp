//a bind function example
#include <iostream>

template<typename ReturnType,typename ArgType>
class BaseFunction{

public:
	virtual ReturnType operator()(ArgType arg)=0;
};

template<typename Class,typename ReturnType,typename ArgType>
class MemberFunction:public BaseFunction<ReturnType,ArgType>{
private:
	typedef ReturnType (Class::*Func)(ArgType);
	Func func;
	Class *obj;
public:
	MemberFunction(Class *obj,Func func):obj(obj),func(func){}
	ReturnType operator()(ArgType arg)
	{
		return (obj->*func)(arg);
	}
};

template<typename ReturnType,typename ArgType>
class Function{
private:
	BaseFunction<ReturnType,ArgType> *func;
public:
	Function(BaseFunction<ReturnType,ArgType> *func):func(func){}
	ReturnType operator()(ArgType arg)
	{
		return func->operator()(arg);
	}


};

class Test{

public:
	int say(int h){
		std::cout<<"say"<<std::endl;
		return 1;
	}
};


template<typename Class,typename ReturnType,typename ArgType>
Function<ReturnType,ArgType> bind(ReturnType (Class::*func)(ArgType),Class *obj)
{
	return new MemberFunction<Class,ReturnType,ArgType>(obj,func);
}
int main(){
	Test t;
	int i=2;
	//MemberFunction<Test,int,int> b(&t,&Test::say);
	Function<int,int> func= bind<Test,int,int>(&Test::say,&t);

	func(1);
	return 0;
}

