#pragma once

//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};

struct Vec4
{
   float x, y, z, w;
};

//4x4 matrix - a standard for graphics.
struct Mat4x4
{
   float values[16];
};

Mat4x4 identityMatrix();

Mat4x4 translateMatrix(Vec2 movement);

Mat4x4 scaleMatrix(Vec2 scale);

Mat4x4 rotateMatrix(float degreeAngle);

Mat4x4 multiplyMatrix(Mat4x4 left, Mat4x4 right);

Mat4x4 transformMatrix(Vec2 movement, float degreeAngle, Vec2 scale);

void printMatrix(Mat4x4 test);