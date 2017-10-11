// ME13 QueueStruct.cpp

#include <iostream>
#include <cstdlib>
#include <cmath>


class Entry{
	std::string data;
	Entry* prev = NULL;
	Entry* next = NULL;
public:
	Entry * getNext(){
		return next;
	};
	Entry * getPrev(){
		return prev;
	};
	std::string getData(){
		return data;
	};
	void setNext(Entry * node){
		next = node;
	};
	void setPrev(Entry * node){
		prev = node;
	};
	void setData(std::string data){
		this->data = data;
	};
};


class QueueStruct{
	Entry* start;
	Entry* end;
	int size;
public:
	QueueStruct();
	void insert(int index, std::string data);
	void remove(int index);
	std::string get(int index);
	void clear();
	void set(int index, std::string data);
	int getSize();
	bool isEmpty();
	bool contains(std::string data);

	void listPrinter();
	
	void enqueue(std::string data);
	void dequeue();
};


QueueStruct::QueueStruct() {
	size = 0;
	start = NULL;
	end = NULL;
}


void QueueStruct::insert(int index, std::string data){
	//check if index is valid
	if(index < 0 || index > size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}
	Entry* handler = new Entry ;
	handler->setData(data);
	if(size==0){

		start = handler;
		end = handler;
		handler->setPrev(NULL);
		handler->setNext(NULL);
	}
	else if(index == 0 ){

		start->setPrev(handler);
		handler->setNext(start);
		start=handler;
	}
	else if (index == size){

		end->setNext(handler);
		handler->setPrev(end);
		end=handler;
	}
	else if(index <= (size/2)){

		handler->setNext(start);
		for(int x=0; x< index; x++){
			handler->setNext(handler->getNext()->getNext());
		}
		handler->setPrev(handler->getNext()->getPrev());
		handler->getPrev()->setNext(handler);
		handler->getNext()->setPrev(handler);
	}
	else if (index > (size/2)){

		handler->setPrev(end);
		for(int x=0; x< std::abs(size-index); x++){
			handler->setPrev(handler->getPrev()->getPrev());
		}
		handler->setNext(handler->getPrev()->getNext());
		handler->getPrev()->setNext(handler);
		handler->getNext()->setPrev(handler);
	}
	size++;
}


void QueueStruct::remove(int index){
	if(index < 0 || index >= size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}

	Entry* ptr;

	if(size == 1){
		delete start;

		start = NULL;
		end = NULL;
	}
	else if(index == 0){
		ptr = start->getNext();
		delete start;
		start = ptr;

	}
	else if(index == size-1){
		ptr = end->getPrev();
		delete end;
		end = ptr;
		end->setNext(NULL);
	}
	else if(index <= (size/2)){

		ptr = start;
		for(int x=0; x< index; x++){
			ptr = ptr->getNext();
		}
		ptr->getPrev()->setNext(ptr->getNext());
		ptr->getNext()->setPrev(ptr->getPrev());
	}
	else if(index > (size/2)){

		ptr = end;
		for(int x=0; x< std::abs(size-index); x++){
			ptr = ptr->getPrev();
		}
		ptr->getPrev()->setNext(ptr->getNext());
		ptr->getNext()->setPrev(ptr->getPrev());
	}
	size--;
}



std::string QueueStruct::get(int index){
	//YSA YSA YSA YSA ASK ASK AYOS AYOS
	if(index < 0 || index >= size){
		//std::cout << "Error: index is not within the size." << std::endl;
		return "Error: index is not within the size.";
	}

	Entry* ptr = start;

	for(int x=0; x< index; x++){
		ptr = ptr->getNext();
	}
	return ptr->getData();
}



void QueueStruct::clear(){
	start = NULL;
	end = NULL;
	size = 0;
	return;
}



void QueueStruct::set(int index, std::string data){
	//YSA YSA YSA YSA ASK ASK AYOS AYOS
	if(index < 0 || index >= size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}

	Entry* ptr = start;

	for(int x=0; x< index; x++){
		ptr = ptr->getNext();
	}
	ptr->setData(data);
	return;
}



int QueueStruct::getSize(){
	return size;
}



bool QueueStruct::isEmpty(){
	if(size==0){
		return 1;
	}
	else{
		return 0;
	}
}



bool QueueStruct::contains(std::string data){
	Entry* ptr = start;


	for(int x=0; x<size; x++){
		if(ptr->getData() == data){
			return 1;
		}
		ptr = ptr->getNext();
	}
	return 0;
}



void QueueStruct::listPrinter(){
	Entry* printer;

	printer = start;
	std::cout <<"LIST::";
	while(printer){
		std::cout << printer->getData() << "   ";
		printer = printer->getNext();
	}
	std::cout << std::endl << std::endl;
}


void QueueStruct::enqueue(std::string data){
	insert(size, data);
}


void QueueStruct::dequeue(){
	remove(size-1);
}

// int main(){
// 	QueueStruct list;

// 	while(1){
// 		int toDo;
// 		std::cout << "[1]:Enqueue a node, [2]:Dequeque a node, [3]:Get a value"<< std::endl;
// 		std::cout << "[4]:Clear List, [5]:Set a value, [6]:Get list size, [7]:Check if list is empty, [8]:Check if value is contained"<< std::endl;
// 		std::cout << std::endl;
// 		std::cout << "What do you want to do?: ";
// 		std::cin >> toDo;


// 		//enqueue
// 		if(toDo==1){
// 			int inIndex;
// 			std::string inData;

// 			std::cout << "Input value of node: ";
// 			std::cin >> inData;
// 			//cin.ignore();
// 			//std::cout << std::endl;

// 			list.enqueue(inData);
// 			list.listPrinter();
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//dequeue
// 		if(toDo==2){
// 			int inIndex;


// 			list.dequeue();
// 			list.listPrinter();
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//get
// 		if(toDo==3){
// 			int inIndex;
// 			std::string value;

// 			std::cout << "Input index number of the value to be taken: ";
// 			std::cin >> inIndex;
// 			std::cout << std::endl;

// 			value = list.get(inIndex);

// 			std::cout << "The value from index " << inIndex << " is: " << value << std::endl;
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//clear
// 		if(toDo==4){
// 			list.clear();
// 			std::cout << "List cleared.";
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//set
// 		if(toDo==5){
// 			int inIndex;
// 			std::string inData;

// 			std::cout << "Input new value of node: ";
// 			std::cin >> inData;
// 			std::cout << std::endl;

// 			std::cout << "Input index number where value should be changed: ";
// 			std::cin >> inIndex;
// 			std::cout << std::endl;

// 			list.set(inIndex, inData);
// 			list.listPrinter();
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//getSize
// 		if(toDo==6){
// 			int outSize;
// 			outSize = list.getSize();

// 			std::cout << "The size of the list is: " << outSize << std::endl;
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//isEmpty
// 		if(toDo==7){
// 			if(list.isEmpty()==1){
// 				std::cout << "The list is empty." << std::endl;
// 			}
// 			else{
// 				std::cout << "The list is not empty." << std::endl;
// 			}
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 		//contains
// 		if(toDo==8){
// 			std::string inData;

// 			std::cout << "Input value of node to be looked for: ";
// 			std::cin >> inData;
// 			std::cout << std::endl;

// 			if(list.contains(inData)==1){
// 				std::cout << "The value " << inData << " is in the list." << std::endl;
// 			}
// 			else{
// 				std::cout << "The value " << inData << " is not in the list." << std::endl;
// 			}
// 			std::cout << std::endl << std::endl << std::endl;
// 		}
// 	}
// //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// }

