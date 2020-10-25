// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <random>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>

typedef unsigned char Byte;

//This resizes a vector and shoves bytes on the end
template <typename T>
void writeStream(std::vector <Byte>& stream, const T& input) {
   int oldSize = stream.size();
   int newSize = oldSize + sizeof(T);
   stream.resize(newSize);
   auto writeLocation = stream.data() + oldSize;
   memcpy(writeLocation, &input, sizeof(T));
}

//Take a vector of things and write the number and then each item
template <typename T>
void writeStream(std::vector <Byte>& stream, std::vector <T> const& input) {
   writeStream(stream, input.size());
   for (auto&& item : input) { writeStream(item); }
}

//This takes a unsigned char* and reads out a chunk, it then increments the stream pointer
template <typename T>
void readStream(Byte*& stream, T& output) {
   memcpy(&output, stream, sizeof(T));
   stream += sizeof(T);
}

//Same as readStream, but for a vector
template <typename T>
void readStream(Byte*& stream, std::vector <T>& output) {
   size_t count;  //size_t can store the size of any object
   readStream(stream, count);
   for (size_t i = 0; i < count; i++) {
      T instance{};
      readStream(stream, instance);
      output.push_back(std::move(instance));  //std::move avoids copying
   }
}

int main(int argc, char* args[])
{
   std::vector <Byte> stream;
   char ipAddress[32] = "127.0.0.1";
   char ipAfter[32];
   writeStream(stream, ipAddress);
   unsigned char* start = stream.data();
   readStream(start, ipAfter);

   printf(ipAfter);
   printf("end");

   return 0;
}