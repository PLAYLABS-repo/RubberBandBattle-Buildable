Absolut::Mesh ground;
void GroundInit(int AmountReachableW, int AmountReachableH){

Absolut::Mesh ground = Absolut::Mesh::CreateCube(1.0f);
ground.CreateCube(1.0f);
ground.position = {0,0,0};
ground.scale = {1, 1, 1};
ground.r = 0.0f;ground.g = 1.0f;ground.b = 0.0f;

}


void GroundDraw(){
ground.draw();

}

