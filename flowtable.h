#include <iostream>
#include <cstdlib>
#include <cmath>

class strArray{
	std::string data[12];
	strArray* prev = NULL;
	strArray* next = NULL;
public:
	strArray * getNext(){
		return next;
	};
	strArray * getPrev(){
		return prev;
	};
	std::string* getData(){
		return data;
	};
	void setNext(strArray * node){
		next = node;
	};
	void setPrev(strArray * node){
		prev = node;
	};
	void setData(std::string data[]){
		this->data[0] = data[0];
		this->data[1] = data[1];
		this->data[2] = data[2];
		this->data[3] = data[3];
		this->data[4] = data[4];
		this->data[5] = data[5];
		this->data[6] = data[6];
		this->data[7] = data[7];
		this->data[8] = data[8];
		this->data[9] = data[9];
		this->data[10] = data[10];
		this->data[11] = data[11];
	};
};

class FlowTable{
	strArray* start;
	strArray* end;
	int size;
public:
	FlowTable();
	void insert(int index, std::string data[]);
	void remove(int index);
	std::string* get(int index);
	void clear();
	void set(int index, std::string data[]);
	int getSize();
	bool isEmpty();
	bool contains(std::string data[]);

	void listPrinter();
};

FlowTable::FlowTable() {
	size = 0;
	start = NULL;
	end = NULL;
}

void FlowTable::insert(int index, std::string data[]){
	//check if index is valid
	if(index < 0 || index > size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}
	strArray* handler = new strArray ;
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

void FlowTable::remove(int index){
	if(index < 0 || index >= size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}

	strArray* ptr;

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

std::string* FlowTable::get(int index){
	//YSA YSA YSA YSA ASK ASK AYOS AYOS
	if(index < 0 || index >= size){
		std::cout << "Error: index is not within the size." << std::endl;
		return NULL;
	}

	strArray* ptr = start;

	for(int x=0; x< index; x++){
		ptr = ptr->getNext();
	}
	return ptr->getData();
}

void FlowTable::clear(){
	start = NULL;
	end = NULL;
	size = 0;
	return;
}

void FlowTable::set(int index, std::string data[]){
	if(index < 0 || index >= size){
		std::cout << "Error: index is not within the size." << std::endl;
		return;
	}
	
	strArray* ptr = start;

	for(int x=0; x< index; x++){
		ptr = ptr->getNext();
	}
	ptr->setData(data);
	return;
}

int FlowTable::getSize(){
	return size;
}

bool FlowTable::isEmpty(){
	if(size==0){
		return 1;
	}
	else{
		return 0;
	}
}

bool FlowTable::contains(std::string data[]){
	strArray* ptr = start;
	std::string* temp =ptr->getData();

	for(int x=0; x<size; x++){
		if(temp[0]==data[0] 
			&& temp[1]==data[1] 
			&& temp[2]==data[2]
			&& temp[3]==data[3]
			&& temp[4]==data[4]
			&& temp[5]==data[5]
			&& temp[6]==data[6]
			&& temp[7]==data[7]
			&& temp[8]==data[8]
			&& temp[9]==data[9]
			&& temp[10]==data[10]
			&& temp[11]==data[11]){
			return 1;
		}
		ptr = ptr->getNext();
		temp = ptr->getData();
	}
	return 0;
}

void FlowTable::listPrinter(){
	strArray* printer;

	printer = start;
	int counter=0;
	while(printer){
		std::string* temp=printer->getData();
		std::cout << counter << ": [" << temp[0] << ", " << temp[1] << ", " << temp[2] << ", " << temp[3] << ", " << temp[4] << ", " << temp[5] << ", " << temp[6] << ", " << temp[7] << ", " << temp[8] << ", " << temp[9] << ", " << temp[10] << ", " << temp[11] << "]\n";
		counter++;
		printer = printer->getNext();
	}
	std::cout << std::endl << std::endl;
}
