#include <string>
#include <iostream>
#include <zmq.hpp>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n) Sleep(n)
#endif


int main(){
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  // Ptr<Socket> dst = socket;


  socket.bind("tcp://*:5555");
  int recvcount = 1;
  while (true){
    zmq::message_t request;
    socket.recv(&request);
    std::cout<<"Received Hello\n";

    sleep(1);
    zmq::message_t reply (5);
    memcpy (reply.data(), "World", 5);
    socket.send(reply);
    recvcount++;
  }
}