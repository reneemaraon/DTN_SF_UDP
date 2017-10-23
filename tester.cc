#include <iostream>
using namespace std;

class bibliografia{
public:
	bibliografia() {}
	virtual void probolh(){
		cout << "from biliografia..." << endl;
	}
};

class sinedria : public bibliografia{
public:
	sinedria(){size = 0;}
	void probolh(){
		cout << "from sinedria..." << endl;
	}
	void ganda(){
		cout <<"ANG GANDA MO\n";
	}
private:
	int size;
};

int main()
{
	bibliografia **test = new bibliografia*[2];
	test[0] = new sinedria;
	test[1] = new sinedria;
	test[0] = dynamic_cast<bibliografia*>(test[0]);
	test[1] = dynamic_cast<bibliografia*>(test[1]);
	test[0]->probolh();
	test[1]->probolh();
	test[0]->ganda();

	return 0;
}

// BaseClass** base = new BaseClass*[2];

// BaseClass[0] = new FirstDerivedClass;
// BaseClass[1] = new SecondDerivedClass;

// template <typename T1, typename T2>
// Ptr<T1>
// DynamicCast (Ptr<T2> const&p)
// {
//   return Ptr<T1> (dynamic_cast<T1 *> (PeekPointer (p)));
// }