#pragma once
#include "GEngine.h"

class Thing : public GameObject
{
  public:
    Thing();

    virtual void Update(float delta);

  private:
    bool key_jump = false;
};
