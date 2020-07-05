// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>


//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};


//4x4 matrix - a standard for graphics.
struct Mat4x4
{
   float values[16];
};

//This will be used to translate, rotate, and scale our meshes
Mat4x4 identityMatrix() {
   Mat4x4 identity;

   for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
         if (row == col) {
            identity.values[4 * row + col] = 1;
         }
         else {
            identity.values[4 * row + col] = 0;
         }
      }
   }

   return identity;
}

//translation
Mat4x4 translateMatrix(Vec2 movement) {
   Mat4x4 out = identityMatrix();
   out.values[3] = movement.x;
   out.values[7] = movement.y;

   return out;
}

//scale
Mat4x4 scaleMatrix(Vec2 scale) {
   Mat4x4 out = identityMatrix();
   out.values[0] = scale.x;
   out.values[5] = scale.y;
   return out;
}

Mat4x4 rotateMatrix(float degreeAngle) {
   Mat4x4 out = identityMatrix();

   float PI = 3.1415926535;
   float radianAngle = PI / 180.0 * degreeAngle;

   out.values[0] = cosf(radianAngle);
   out.values[1] = -sinf(radianAngle);
   out.values[4] = sinf(radianAngle);
   out.values[5] = cosf(radianAngle);

   return out;
}

Mat4x4 multiplyMatrix(Mat4x4 left, Mat4x4 right) {
   Mat4x4 out;

   //loop through left matrix
   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
         float sum = 0;
         for (int k = 0; k < 4; k++) {
            sum += left.values[4 * i + k] * right.values[4 * k + j];
         }
         out.values[4 * i + j] = sum;
      }
   }
   return out;
}

//translate, rotate, scale together
Mat4x4 transformMatrix(Vec2 movement, float degreeAngle, Vec2 scale) {
   Mat4x4 intermediate = multiplyMatrix(translateMatrix(movement), rotateMatrix(degreeAngle));
   Mat4x4 out = multiplyMatrix(intermediate, scaleMatrix(scale));
   return out;
}

void printMatrix(Mat4x4 test) {

   for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
         printf("%f, ", test.values[4 * row + col]);
      }
      printf("\n");
   }
   printf("\n");
}

int main()
{
   //Mat4x4 identity = identityMatrix();
   //printMatrix(identity);

   //Vec2 scaleValue = { 2, 2 };
   //Mat4x4 scale = scaleMatrix(scaleValue);
   //printMatrix(scale);

   //float degreeAngle = 45.0;
   //Mat4x4 rotate = rotateMatrix(degreeAngle);
   //printMatrix(rotate);

   //Vec2 movement = { 1, 1 };
   //Mat4x4 translate = translateMatrix(movement);
   //printMatrix(translate);

   //Mat4x4 multiply = multiplyMatrix(scale, rotate);
   //printMatrix(multiply);

   //Mat4x4 transform = transformMatrix(movement, degreeAngle, scaleValue);
   //printMatrix(transform);

   Vec2 botLeft = { -1, -1 };
   Vec2 botRight = { 1, -1 };
   Vec2 topLeft = { -1, 1 };
   Vec2 topRight = { 1, 1 };

   Vec2 worldBotLeft = { 0, 0 };
   Vec2 worldTopRight = { 900, 1200 };

   //device to world
   Vec2 movement = { (worldBotLeft.x - botLeft.x), (worldBotLeft.y - botLeft.y) };
   Vec2 scale = { (worldTopRight.x - worldBotLeft.x) / (topRight.x - botLeft.x), (worldTopRight.y - worldBotLeft.y) / (topRight.y - botLeft.y) };

   Mat4x4 mMove = translateMatrix(movement);
   Mat4x4 mScale = scaleMatrix(scale);

   Mat4x4 transform = multiplyMatrix(mScale, mMove);

   //Mat4x4 transform = transformMatrix(movement, 0, scale);
   printMatrix(transform);

   //world to device
   Vec2 movement2 = { (botLeft.x - worldBotLeft.x), (botLeft.y - worldBotLeft.y) };
   Vec2 scale2 = { (topRight.x - botLeft.x) / (worldTopRight.x - worldBotLeft.x), (topRight.y - botLeft.y) / (worldTopRight.y - worldBotLeft.y) };

   Mat4x4 mMove2 = translateMatrix(movement2);
   Mat4x4 mScale2 = scaleMatrix(scale2);

   Mat4x4 transform2 = multiplyMatrix(mMove2, mScale2);

   //Mat4x4 transform2 = transformMatrix(movement2, 0, scale2);
   printMatrix(transform2);
}