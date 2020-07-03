

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
            identity.values[row + col] = 1;
         }
         else {
            identity.values[row + col] = 0;
         }
      }
   }
}

//translation
Mat4x4 translateMatrix(Vec2 movement) {
   Mat4x4 out;

}
