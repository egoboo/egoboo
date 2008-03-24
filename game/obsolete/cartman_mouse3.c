int             msx, msy, msxold, msyold, msb, mcx, mcy;
int		mstlx, mstly, msbrx, msbry;
int		mousespeed = 2;

//------------------------------------------------------------------------------
void moson(void)
{
  install_mouse();
  show_mouse(NULL);
  set_mouse_speed(mousespeed, mousespeed);


  return;
}

//------------------------------------------------------------------------------
void mosdo(void)
{
  msxold = msx;
  msyold = msy;
  get_mouse_mickeys(&mcx, &mcy);
  msx+=mcx;
  msy+=mcy;
  msb = mouse_b;
//  limit(0, &msx, OUTX-1);
//  limit(0, &msy, OUTY-1);
  limit(mstlx, &msx, msbrx);
  limit(mstly, &msy, msbry);

  return;
}

//------------------------------------------------------------------------------
