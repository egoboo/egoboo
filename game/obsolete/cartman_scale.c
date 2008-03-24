#define MAXSAMP 640                     // Maximum output size scaler can do
#define THRESHSAMP 12                   // Number of hits needed for solid
#define CLEARCOLOR 0			// Transparent color
//------------------------------------------------------------------------------
//void scale_blit(GLtexture* bmpsource, GLtexture* bmpdest, int light)
//{
//  int width, height;
//  int nuwidth, nuheight;
//  int xsamp[MAXSAMP][4];
//  int ysamp[MAXSAMP][4];
//  unsigned int red, grn, blu;
//  register unsigned short xxx, yyy;
//  register unsigned short xcnt, ycnt, scnt;
//  unsigned char samp;
//
//
//  width = (*bmpsource).w;
//  height = (*bmpsource).h;
//  nuwidth = (*bmpdest).w;
//  nuheight = (*bmpdest).h;
//
//
//  if(width == nuwidth && height == nuheight)
//  {
//    // Just copy 'em
//    blit(bmpsource, bmpdest, 0, 0, 0, 0,
//                              width, height);
//    return;
//  }
//  if(nuwidth > MAXSAMP || nuheight > MAXSAMP)
//  {
//    // Clear it 'cause it's too big
//    clear(bmpdest);
//    return;
//  }
//
//
//
//  // Fill in where to sample from ( 4 for each )
//  xxx = 0;
//  while(xxx < (nuwidth<<2))
//  {
//    xsamp[xxx>>2][xxx&3] = (xxx * width)/(nuwidth<<2);
//    xxx++;
//  }
//  yyy = 0;
//  while(yyy < (nuheight<<2))
//  {
//    ysamp[yyy>>2][yyy&3] = (yyy * height)/(nuheight<<2);
//    yyy++;
//  }
//
//
//  yyy = 0;
//  while(yyy < nuheight)
//  {
//    xxx = 0;
//    while(xxx < nuwidth)
//    {
//      red = 0; grn = 0; blu = 0;
//      scnt = 0;
//      ycnt = 0;
//      while(ycnt < 4)
//      {      
//        xcnt = 0;
//        while(xcnt < 4)
//        {
//          samp = _getpixel(bmpsource, xsamp[xxx][xcnt], ysamp[yyy][ycnt]);
//          if(samp != CLEARCOLOR)
//          {
//            red += goodpal[samp].r;  grn += goodpal[samp].g;  blu += goodpal[samp].b;
//            scnt++;
//          } 
//          xcnt++;
//        }
//        ycnt++;
//      }
//
//
//      samp = 0;
//      if(scnt > THRESHSAMP)
//      {
//        red = red*light>>8;  grn = grn*light>>8;  blu = blu*light>>8;
//        red = red/scnt;  grn = grn/scnt;  blu = blu/scnt;
//        samp = tricolor.data[red>>1][grn>>1][blu>>1];
//      }
//      _putpixel(bmpdest, xxx, yyy, samp);
//
//
//      xxx++;
//    }
//    yyy++;
//  }
//
//
//  return;
//}
//
////------------------------------------------------------------------------------
//void scale_quick(GLtexture* bmpsource, GLtexture* bmpdest)
//{
//  int width, height;
//  int nuwidth, nuheight;
//  int xsamp[MAXSAMP][4];
//  int ysamp[MAXSAMP][4];
//  unsigned short xxx, yyy;
//  register unsigned short red, grn, blu;
//  register unsigned short xcnt, ycnt;
//  unsigned char samp;
//
//
//  width = (*bmpsource).w;
//  height = (*bmpsource).h;
//  nuwidth = (*bmpdest).w;
//  nuheight = (*bmpdest).h;
//
//
//  if(width == nuwidth && height == nuheight)
//  {
//    // Just copy 'em
//    blit(bmpsource, bmpdest, 0, 0, 0, 0,
//                              width, height);
//    return;
//  }
//  if(nuwidth > MAXSAMP || nuheight > MAXSAMP)
//  {
//    // Clear it 'cause it's too big
//    clear(bmpdest);
//    return;
//  }
//
//
//
//  // Fill in where to sample from ( 4 for each )
//  xxx = 0;
//  while(xxx < (nuwidth<<2))
//  {
//    xsamp[xxx>>2][xxx&3] = (xxx * width)/(nuwidth<<2);
//    xxx++;
//  }
//  yyy = 0;
//  while(yyy < (nuheight<<2))
//  {
//    ysamp[yyy>>2][yyy&3] = (yyy * height)/(nuheight<<2);
//    yyy++;
//  }
//
//
//  yyy = 0;
//  while(yyy < nuheight)
//  {
//    xxx = 0;
//    while(xxx < nuwidth)
//    {
//      red = 0; grn = 0; blu = 0;
//      ycnt = 0;
//      while(ycnt < 4)
//      {      
//        xcnt = 0;
//        while(xcnt < 4)
//        {
//          samp = _getpixel(bmpsource, xsamp[xxx][xcnt], ysamp[yyy][ycnt]);
//          red += goodpal[samp].r;  grn += goodpal[samp].g;  blu += goodpal[samp].b;
//          xcnt++;
//        }
//        ycnt++;
//      }
//
//
//      samp = tricolor.data[red>>5][grn>>5][blu>>5];
//      _putpixel(bmpdest, xxx, yyy, samp);
//
//
//      xxx++;
//    }
//    yyy++;
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
