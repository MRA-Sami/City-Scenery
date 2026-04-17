//Final Project //
/*
 * ============================================================
 *  CITY SCENE — OpenGL / GLUT  (C++)
 *  Features:
 *    • Day / Night toggle with sky gradient & stars
 *    • Bresenham Line & Midpoint Circle algorithms
 *    • Moving vehicles (cars & buses) in fixed lanes
 *    • Walking pedestrians & students → Daffodil School
 *    • City Hospital with red cross
 *    • Blinking traffic lights
 *    • Street lights that glow at night
 *    • Animated clouds, sun, moon
 *    • Bonus: rain effect (Bresenham lines), billboard, park
 *
 *  Keyboard Controls:
 *    D – Day mode        N – Night mode
 *    S – Start/Stop      C – Toggle clouds
 *    L – Street lights   P – Toggle people
 *    V – Toggle vehicles R – Toggle rain
 *    ESC – Quit
 *
 *  Compile (Linux):
 *    g++ city_scene.cpp -o city_scene -lGL -lGLU -lglut -lm
 *  Compile (Windows MinGW):
 *    g++ city_scene.cpp -o city_scene.exe -lfreeglut -lopengl32 -lglu32
 * ============================================================
 */

#ifdef _WIN32
  #include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

// ── Window ───────────────────────────────────────────────────
static const int W = 1100, H = 700;
#define PI 3.14159265358979f

// ── Global state ─────────────────────────────────────────────
bool gNight      = false;
bool gAnimating  = true;
bool gClouds     = true;
bool gLights     = true;
bool gPeople     = true;
bool gVehicles   = true;
bool gRain       = false;

int  gFrame      = 0;
int  gTLTimer    = 0;
int  gTLState    = 2;   // 0=red 1=yellow 2=green

float gSunX  = 80.f,  gSunY  = 590.f;
float gMoonX = -80.f, gMoonY = 590.f;

// ── Data types ───────────────────────────────────────────────
struct Vehicle {
    float x, y, speed;
    int   type;          // 0=car  1=bus
    bool  rightward;
    unsigned char r, g, b;
};

struct Person {
    float x, y, speed, leg;
    bool  rightward, student;
};

struct Cloud { float x, y, spd; };
struct Drop  { float x, y, spd; };

static std::vector<Vehicle> gCars;
static std::vector<Person>  gWalkers, gStudents;
static std::vector<Cloud>   gCloudList;
static std::vector<Drop>    gDrops;

// ═══════════════════════════════════════════════════════════════
//  GRAPHICS ALGORITHMS
// ═══════════════════════════════════════════════════════════════

/* Bresenham Line — draws with GL_POINTS so it respects current color */
void bresenhamLine(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2-x1), dy = abs(y2-y1);
    int sx = (x1<x2)?1:-1, sy = (y1<y2)?1:-1;
    int err = dx - dy;
    glBegin(GL_POINTS);
    for(;;){
        glVertex2i(x1, y1);
        if(x1==x2 && y1==y2) break;
        int e2 = 2*err;
        if(e2 > -dy){ err -= dy; x1 += sx; }
        if(e2 <  dx){ err += dx; y1 += sy; }
    }
    glEnd();
}

/* Midpoint Circle — filled=false → outline only */
void midpointCircle(int cx, int cy, int R, bool filled)
{
    if(R <= 0) return;
    int x=0, y=R, d=1-R;
    auto hline=[&](int xa,int xb,int yy){
        glBegin(GL_LINES);
        glVertex2i(xa,yy); glVertex2i(xb,yy);
        glEnd();
    };
    while(x<=y){
        if(filled){
            hline(cx-y,cx+y,cy+x); hline(cx-y,cx+y,cy-x);
            hline(cx-x,cx+x,cy+y); hline(cx-x,cx+x,cy-y);
        } else {
            glBegin(GL_POINTS);
            glVertex2i(cx+x,cy+y); glVertex2i(cx-x,cy+y);
            glVertex2i(cx+x,cy-y); glVertex2i(cx-x,cy-y);
            glVertex2i(cx+y,cy+x); glVertex2i(cx-y,cy+x);
            glVertex2i(cx+y,cy-x); glVertex2i(cx-y,cy-x);
            glEnd();
        }
        if(d<0){ d+=2*x+3; }
        else   { d+=2*(x-y)+5; --y; }
        ++x;
    }
}

// ═══════════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════════
void text(float x, float y, const char* s, void* font=GLUT_BITMAP_HELVETICA_12)
{
    glRasterPos2f(x,y);
    for(; *s; ++s) glutBitmapCharacter(font, *s);
}

void quad(float x,float y,float w,float h){ // axis-aligned filled quad
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

// ═══════════════════════════════════════════════════════════════
//  SKY
// ═══════════════════════════════════════════════════════════════
void drawSky()
{
    if(!gNight){
        glBegin(GL_QUADS);
        glColor3f(0.35f,0.65f,1.0f); glVertex2f(0,H);   glVertex2f(W,H);
        glColor3f(0.65f,0.88f,1.0f); glVertex2f(W,300);  glVertex2f(0,300);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.01f,0.01f,0.10f); glVertex2f(0,H);  glVertex2f(W,H);
        glColor3f(0.03f,0.03f,0.18f); glVertex2f(W,300); glVertex2f(0,300);
        glEnd();
        // Stars
        glColor3f(1,1,1); glPointSize(2.f);
        glBegin(GL_POINTS);
        unsigned int s=0x12345678u;
        for(int i=0;i<130;++i){
            s^=s<<13; s^=s>>17; s^=s<<5;
            float sx=(s&0x7FFF)/(float)0x7FFF * W;
            float sy=310+(s>>16&0x7FFF)/(float)0x7FFF *(H-310);
            glVertex2f(sx,sy);
        }
        glEnd();
        glPointSize(1.f);
    }
}

// ═══════════════════════════════════════════════════════════════
//  SUN / MOON
// ═══════════════════════════════════════════════════════════════
void drawSunMoon()
{
    if(!gNight){
        // Glow rings
        glColor4f(1.0f,0.92f,0.3f,0.18f); midpointCircle((int)gSunX,(int)gSunY,52,true);
        glColor3f(1.0f,0.90f,0.15f);       midpointCircle((int)gSunX,(int)gSunY,38,true);
        glColor3f(1.0f,1.00f,0.55f);       midpointCircle((int)gSunX,(int)gSunY,26,true);
        // Rays
        glColor3f(1.0f,0.85f,0.0f);
        for(int i=0;i<8;++i){
            float a=i*45*PI/180.f;
            bresenhamLine((int)(gSunX+40*cosf(a)),(int)(gSunY+40*sinf(a)),
                          (int)(gSunX+58*cosf(a)),(int)(gSunY+58*sinf(a)));
        }
    } else {
        // Moon body
        glColor3f(0.95f,0.95f,0.85f);
        midpointCircle((int)gMoonX,(int)gMoonY,32,true);
        // Craters
        glColor3f(0.80f,0.80f,0.75f);
        midpointCircle((int)gMoonX+9,(int)gMoonY+9,7,true);
        midpointCircle((int)gMoonX-11,(int)gMoonY-5,4,true);
        midpointCircle((int)gMoonX+3,(int)gMoonY-12,3,true);
    }
}

// ═══════════════════════════════════════════════════════════════
//  CLOUDS
// ═══════════════════════════════════════════════════════════════
void drawClouds()
{
    if(!gClouds || gNight) return;
    for(auto& c: gCloudList){
        glColor3f(1,1,1);
        midpointCircle((int)c.x,   (int)c.y,   26,true);
        midpointCircle((int)c.x+32,(int)c.y+ 6, 21,true);
        midpointCircle((int)c.x-32,(int)c.y+ 6, 18,true);
        midpointCircle((int)c.x+16,(int)c.y+16, 23,true);
        midpointCircle((int)c.x-16,(int)c.y+14, 20,true);
    }
}

// ═══════════════════════════════════════════════════════════════
//  GROUND
// ═══════════════════════════════════════════════════════════════
void drawGround()
{
    glColor3f(gNight?0.12f:0.28f, gNight?0.30f:0.58f, gNight?0.12f:0.20f);
    quad(0,0,W,312);
}

// ═══════════════════════════════════════════════════════════════
//  ROAD
// ═══════════════════════════════════════════════════════════════
void drawRoad()
{
    // Asphalt
    glColor3f(0.22f,0.22f,0.22f);
    quad(0,148,W,116);   // main road y148..264

    // Curbs
    glColor3f(0.75f,0.75f,0.75f);
    quad(0,145,W,5); quad(0,261,W,5);

    // Upper sidewalk
    glColor3f(gNight?0.45f:0.68f, gNight?0.45f:0.68f, gNight?0.42f:0.63f);
    quad(0,108,W,42);   // y108..150

    // Lower sidewalk
    glColor3f(gNight?0.45f:0.68f, gNight?0.45f:0.68f, gNight?0.42f:0.63f);
    quad(0,263,W,42);   // y263..305

    // Center yellow dashes — Bresenham
    glColor3f(1.0f,0.9f,0.0f);
    for(int x=0; x<W; x+=42)
        bresenhamLine(x,206,x+22,206);

    // White edge lines
    glColor3f(0.9f,0.9f,0.9f);
    bresenhamLine(0,175,W,175);
    bresenhamLine(0,237,W,237);

    // Zebra crossing near school  x≈320
    // ================= ZEBRA CROSSING (IMPROVED) =================

// function-like block for reuse
auto drawZebra = [&](float startX){
    float stripeWidth = 12;
    float gap = 11;
    int count = 3;

    for(int i=0; i<count; i++){
        float x = startX + i * (stripeWidth + gap);

        // main stripe
        glColor3f(1.0f, 1.0f, 1.0f);
        quad(x, 148, stripeWidth, 117);

        // subtle shadow effect (depth)
        glColor4f(0.0f, 0.0f, 0.0f, 0.15f);
        quad(x+2, 148, stripeWidth, 117);
    }
};

// near school
drawZebra(320);

// near hospital
drawZebra(710);

// Zebra crossing near hospital
for(int i=0;i<8;++i)
    bresenhamLine(712+i*11,148,712+i*11,265);

    // Zebra crossing near hospital  x≈710
    for(int i=0;i<8;++i)
        bresenhamLine(712+i*11,148,712+i*11,265);
}

// ═══════════════════════════════════════════════════════════════
//  SINGLE BUILDING
// ═══════════════════════════════════════════════════════════════
void drawBuilding(float x,float y,float w,float h,
                  float r,float g,float b)
{
    // Shadow
    glColor4f(0,0,0,0.22f);
    quad(x+6,y-6,w,6);

    // Body
    glColor3f(r,g,b);
    quad(x,y,w,h);

    // Side face (depth illusion)
    glBegin(GL_QUADS);
    glColor3f(r*0.65f,g*0.65f,b*0.65f);
    glVertex2f(x+w,y);   glVertex2f(x+w+8,y+6);
    glVertex2f(x+w+8,y+h+6); glVertex2f(x+w,y+h);
    glEnd();

    // Roof
    glColor3f(r*0.72f,g*0.72f,b*0.72f);
    glBegin(GL_QUADS);
    glVertex2f(x,y+h); glVertex2f(x+w,y+h);
    glVertex2f(x+w+8,y+h+6); glVertex2f(x+8,y+h+6);
    glEnd();

    // Edges — Bresenham
    glColor3f(r*0.55f,g*0.55f,b*0.55f);
    bresenhamLine((int)x,(int)y,(int)(x+w),(int)y);
    bresenhamLine((int)(x+w),(int)y,(int)(x+w),(int)(y+h));
    bresenhamLine((int)x,(int)y,(int)x,(int)(y+h));

    // Windows
    int cols=(int)(w/22); if(cols<1)cols=1;
    int rows=(int)(h/28); if(rows<1)rows=1;
    float gx=w/(cols+1), gy=h/(rows+1);
    for(int c=1;c<=cols;++c){
        for(int rv=1;rv<=rows;++rv){
            float wx=x+c*gx-5, wy=y+rv*gy-5;
            bool lit = gNight && gLights && ((c+rv)%3!=0);
            if(lit)             glColor3f(1.0f,0.95f,0.55f);
            else if(gNight)     glColor3f(0.08f,0.08f,0.12f);
            else                glColor3f(0.55f,0.82f,1.0f);
            quad(wx,wy,11,11);
            glColor3f(r*0.5f,g*0.5f,b*0.5f);
            bresenhamLine((int)wx+5,(int)wy,(int)wx+5,(int)wy+11);
            bresenhamLine((int)wx,(int)wy+5,(int)wx+11,(int)wy+5);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  ALL BUILDINGS
// ═══════════════════════════════════════════════════════════════
void drawBuildings()
{
    float dm = gNight ? 0.55f : 1.0f;
    // Far-left cluster
    drawBuilding(  5,263, 68,195, 0.52f*dm,0.52f*dm,0.58f*dm);
    drawBuilding( 82,263, 52,168, 0.60f*dm,0.55f*dm,0.48f*dm);
    drawBuilding(143,263, 60,215, 0.48f*dm,0.54f*dm,0.64f*dm);
    drawBuilding(213,263, 48,148, 0.64f*dm,0.60f*dm,0.52f*dm);
    // Mid-right cluster (behind hospital / billboard zone)
    drawBuilding(455,263, 58,178, 0.50f*dm,0.60f*dm,0.70f*dm);
    drawBuilding(522,263, 44,158, 0.70f*dm,0.64f*dm,0.50f*dm);
    drawBuilding(575,263, 68,198, 0.53f*dm,0.60f*dm,0.53f*dm);
    // Far-right
    drawBuilding(782,263, 52,182, 0.60f*dm,0.50f*dm,0.60f*dm);
    drawBuilding(843,263, 62,142, 0.50f*dm,0.60f*dm,0.65f*dm);
    drawBuilding(916,263, 68,208, 0.65f*dm,0.60f*dm,0.50f*dm);
    drawBuilding(993,263, 98,188, 0.55f*dm,0.58f*dm,0.62f*dm);
}

// ═══════════════════════════════════════════════════════════════
//  SCHOOL
// ═══════════════════════════════════════════════════════════════
void drawSchool()
{
    float sx=278, sy=263, sw=125, sh=132;
    float dm=gNight?0.6f:1.0f;

    // Shadow
    glColor4f(0,0,0,0.22f); quad(sx+7,sy-7,sw,7);

    // Body
    glColor3f(0.96f*dm,0.86f*dm,0.50f*dm); quad(sx,sy,sw,sh);

    // Roof triangle
    glBegin(GL_TRIANGLES);
    glColor3f(0.72f*dm,0.22f*dm,0.22f*dm);
    glVertex2f(sx-10,sy+sh); glVertex2f(sx+sw+10,sy+sh); glVertex2f(sx+sw/2,sy+sh+44);
    glEnd();

    // Flagpole
    glColor3f(0.4f,0.4f,0.4f);
    bresenhamLine((int)(sx+sw/2),(int)(sy+sh+44),(int)(sx+sw/2),(int)(sy+sh+90));
    // Flag
    glBegin(GL_TRIANGLES);
    glColor3f(0.0f,0.45f,1.0f);
    glVertex2f(sx+sw/2,sy+sh+90); glVertex2f(sx+sw/2+22,sy+sh+82); glVertex2f(sx+sw/2,sy+sh+74);
    glEnd();

    // Edges
    glColor3f(0.72f*dm,0.62f*dm,0.32f*dm);
    bresenhamLine((int)sx,(int)sy,(int)(sx+sw),(int)sy);
    bresenhamLine((int)(sx+sw),(int)sy,(int)(sx+sw),(int)(sy+sh));
    bresenhamLine((int)sx,(int)sy,(int)sx,(int)(sy+sh));

    // Door
    glColor3f(0.38f*dm,0.23f*dm,0.10f*dm); quad(sx+sw/2-13,sy,26,36);
    glColor3f(0.9f,0.7f,0.1f); midpointCircle((int)(sx+sw/2+8),(int)(sy+18),3,true);

    // Windows
    for(int i=0;i<2;++i){
        float wx=sx+10+i*72, wy=sy+54;
        if(gNight&&gLights) glColor3f(1.0f,0.95f,0.6f);
        else if(gNight)     glColor3f(0.08f,0.08f,0.12f);
        else                glColor3f(0.6f,0.84f,1.0f);
        quad(wx,wy,32,32);
        glColor3f(0.4f,0.4f,0.4f);
        bresenhamLine((int)wx+16,(int)wy,(int)wx+16,(int)wy+32);
        bresenhamLine((int)wx,(int)wy+16,(int)wx+32,(int)wy+16);
    }

    // Name plate
    glColor3f(0.10f,0.25f,0.70f); quad(sx+4,sy+sh-20,sw-8,16);
    glColor3f(1,1,1);
    text(sx+9,sy+sh-10," ",GLUT_BITMAP_HELVETICA_10);

    // Steps
    glColor3f(0.58f*dm,0.58f*dm,0.56f*dm);
    quad(sx+sw/2-22,sy-9,44,9);
    glColor3f(0.64f*dm,0.64f*dm,0.62f*dm);
    quad(sx+sw/2-16,sy-17,32,9);
}

// ═══════════════════════════════════════════════════════════════
//  HOSPITAL
// ═══════════════════════════════════════════════════════════════
void drawHospital()
{
    float hx=658, hy=263, hw=112, hh=122;
    float dm=gNight?0.65f:1.0f;

    // Shadow
    glColor4f(0,0,0,0.22f); quad(hx+7,hy-7,hw,7);

    // Body
    glColor3f(0.95f*dm,0.95f*dm,0.95f*dm); quad(hx,hy,hw,hh);

    // Roof parapet
    glColor3f(0.58f*dm,0.58f*dm,0.68f*dm); quad(hx-6,hy+hh,hw+12,14);

    // Edges
    glColor3f(0.68f*dm,0.68f*dm,0.72f*dm);
    bresenhamLine((int)hx,(int)hy,(int)(hx+hw),(int)hy);
    bresenhamLine((int)(hx+hw),(int)hy,(int)(hx+hw),(int)(hy+hh));
    bresenhamLine((int)hx,(int)hy,(int)hx,(int)(hy+hh));

    // Red Cross (vertical + horizontal bars)
    glColor3f(0.88f*dm,0.10f,0.10f);
    quad(hx+hw/2-9,hy+hh-58,18,44);   // vertical bar
    quad(hx+hw/2-24,hy+hh-48,48,16);  // horizontal bar

    // Door
    glColor3f(0.38f*dm,0.38f*dm,0.60f*dm); quad(hx+hw/2-13,hy,26,32);

    // Windows
    for(int i=0;i<2;++i){
        float wy=hy+38+i*36;
        for(int j=0;j<2;++j){
            float wx=hx+8+j*58;
            if(gNight&&gLights) glColor3f(1.0f,0.95f,0.6f);
            else if(gNight)     glColor3f(0.08f,0.08f,0.12f);
            else                glColor3f(0.6f,0.84f,1.0f);
            quad(wx,wy,22,22);
        }
    }

    // Name plate
    glColor3f(0.80f*dm,0.08f,0.08f); quad(hx+5,hy+hh-14,hw-10,12);
    glColor3f(1,1,1);
    text(hx+16,hy+hh-5," ",GLUT_BITMAP_HELVETICA_10);

    // Steps
    glColor3f(0.60f*dm,0.60f*dm,0.64f*dm);
    quad(hx+hw/2-22,hy-9,44,9);
}

// ═══════════════════════════════════════════════════════════════
//  TREE
// ═══════════════════════════════════════════════════════════════
void drawTree(float x,float y)
{
    float dm=gNight?0.45f:1.0f;
    // Shadow
    glColor4f(0,0,0,0.18f); quad(x-16,y-4,34,6);
    // Trunk
    glColor3f(0.40f*dm,0.25f*dm,0.10f*dm); quad(x-5,y,10,32);
    // Foliage — midpoint circles
    glColor3f(0.08f*dm,0.46f*dm,0.08f*dm); midpointCircle((int)x,(int)y+52,27,true);
    glColor3f(0.12f*dm,0.58f*dm,0.12f*dm); midpointCircle((int)x-5,(int)y+68,21,true);
                                            midpointCircle((int)x+5,(int)y+68,19,true);
    glColor3f(0.18f*dm,0.70f*dm,0.18f*dm); midpointCircle((int)x,(int)y+80,15,true);
}

void drawTrees()
{
    const float ty = 268.f;
    float xs[]={55,138,235,428,555,638,770,858,962};
    for(float x: xs) drawTree(x,ty);
}

// ═══════════════════════════════════════════════════════════════
//  STREET LIGHT
// ═══════════════════════════════════════════════════════════════
void drawStreetLight(float x,float y)
{
    float dm=gNight?0.6f:1.0f;
    // Pole
    glColor3f(0.38f*dm,0.38f*dm,0.42f*dm);
    quad(x-3,y,6,62);
    // Arm
    quad(x,y+60,22,3);
    // Shade
    glBegin(GL_TRIANGLES);
    glColor3f(0.28f*dm,0.28f*dm,0.32f*dm);
    glVertex2f(x+14,y+62); glVertex2f(x+30,y+62); glVertex2f(x+22,y+68);
    glEnd();
    // Bulb
    if(gNight && gLights){
        glColor4f(1.0f,0.92f,0.30f,0.28f); midpointCircle((int)(x+22),(int)(y+57),14,true);
        glColor3f(1.0f,0.95f,0.50f);       midpointCircle((int)(x+22),(int)(y+57), 9,true);
        glColor3f(1.0f,1.00f,0.75f);       midpointCircle((int)(x+22),(int)(y+57), 5,true);
    } else {
        glColor3f(0.80f,0.80f,0.60f);
        midpointCircle((int)(x+22),(int)(y+57),9,true);
    }
}

// ═══════════════════════════════════════════════════════════════
//  TRAFFIC LIGHT
// ═══════════════════════════════════════════════════════════════
void drawTrafficLight(float x,float y)
{
    // Pole
    glColor3f(0.30f,0.30f,0.34f); quad(x-3,y,6,72);
    // Box
    glColor3f(0.12f,0.12f,0.12f); quad(x-13,y+72,26,46);
    // Lights
    glColor3f(gTLState==0?1.0f:0.28f, 0.0f, 0.0f);
    midpointCircle((int)x,(int)(y+112),8,true);
    glColor3f(gTLState==1?1.0f:0.28f, gTLState==1?0.88f:0.28f, 0.0f);
    midpointCircle((int)x,(int)(y+ 96),8,true);
    glColor3f(0.0f, gTLState==2?1.0f:0.18f, 0.0f);
    midpointCircle((int)x,(int)(y+ 80),8,true);
}

// ═══════════════════════════════════════════════════════════════
//  CAR
// ═══════════════════════════════════════════════════════════════
void drawCar(float cx,float cy,unsigned char R,unsigned char G,unsigned char B,bool flip)
{
    glPushMatrix();
    glTranslatef(cx,cy,0);
    if(flip) glScalef(-1,1,1);

    // Shadow
    glColor4f(0,0,0,0.22f); quad(-24,-4,48,5);

    // Body
    glColor3ub(R,G,B); quad(-25,0,50,15);
    // Roof
    glColor3ub((unsigned char)(R*0.82),(unsigned char)(G*0.82),(unsigned char)(B*0.82));
    glBegin(GL_QUADS);
    glVertex2f(-13,15); glVertex2f(16,15); glVertex2f(13,25); glVertex2f(-10,25);
    glEnd();
    // Windshield
    glColor4f(0.60f,0.84f,1.0f,0.85f);
    glBegin(GL_QUADS);
    glVertex2f( 8,15); glVertex2f(16,15); glVertex2f(13,24); glVertex2f(6,24);
    glEnd();
    // Rear window
    glBegin(GL_QUADS);
    glVertex2f(-13,15); glVertex2f(-6,15); glVertex2f(-9,24); glVertex2f(-12,24);
    glEnd();

    // Wheels — midpoint circle
    glColor3f(0.1f,0.1f,0.1f);
    midpointCircle(-15,0,7,true); midpointCircle(15,0,7,true);
    glColor3f(0.58f,0.58f,0.58f);
    midpointCircle(-15,0,4,true); midpointCircle(15,0,4,true);

    // Lights
    glColor3f(gNight?1.0f:0.9f, gNight?1.0f:0.9f, gNight?0.7f:0.6f);
    midpointCircle(24,7,4,true);
    glColor3f(0.9f,0.2f,0.2f);
    midpointCircle(-24,7,3,true);

    glPopMatrix();
}

// ═══════════════════════════════════════════════════════════════
//  BUS
// ═══════════════════════════════════════════════════════════════
void drawBus(float cx,float cy,unsigned char R,unsigned char G,unsigned char B,bool flip)
{
    glPushMatrix();
    glTranslatef(cx,cy,0);
    if(flip) glScalef(-1,1,1);

    // Shadow
    glColor4f(0,0,0,0.22f); quad(-40,-5,80,6);

    // Body
    glColor3ub(R,G,B); quad(-40,0,80,28);
    // Roof strip
    glColor3ub((unsigned char)(R*0.78),(unsigned char)(G*0.78),(unsigned char)(B*0.78));
    quad(-40,28,80,6);

    // Windows row
    for(int i=-3;i<=3;++i){
        if(gNight&&gLights) glColor3f(1.0f,0.95f,0.60f);
        else if(gNight)     glColor3f(0.08f,0.08f,0.12f);
        else                glColor4f(0.6f,0.84f,1.0f,0.85f);
        quad(i*12-5,17,10,9);
    }
    // Door
    glColor3ub((unsigned char)(R*0.68),(unsigned char)(G*0.68),(unsigned char)(B*0.68));
    quad(28,0,11,20);

    // Wheels
    glColor3f(0.1f,0.1f,0.1f);
    midpointCircle(-24,0,9,true); midpointCircle(24,0,9,true);
    glColor3f(0.5f,0.5f,0.55f);
    midpointCircle(-24,0,5,true); midpointCircle(24,0,5,true);

    // Lights
    glColor3f(gNight?1.0f:0.9f, gNight?1.0f:0.9f, gNight?0.7f:0.6f);
    midpointCircle(39, 8,4,true); midpointCircle(39,20,4,true);
    glColor3f(0.9f,0.2f,0.2f);
    midpointCircle(-39,14,4,true);

    glPopMatrix();
}

// ═══════════════════════════════════════════════════════════════
//  DRAW ALL VEHICLES
// ═══════════════════════════════════════════════════════════════
void drawVehicles()
{
    for(auto& v: gCars){
        if(v.type==0) drawCar(v.x,v.y,v.r,v.g,v.b,!v.rightward);
        else          drawBus(v.x,v.y,v.r,v.g,v.b,!v.rightward);
    }
}

// ═══════════════════════════════════════════════════════════════
//  PERSON
// ═══════════════════════════════════════════════════════════════
void drawPerson(float px,float py,float leg,bool student,bool right)
{
    glPushMatrix();
    glTranslatef(px,py,0);
    if(!right) glScalef(-1,1,1);
    float sc=student?0.82f:1.0f;
    glScalef(sc,sc,1);

    // Shadow
    glColor4f(0,0,0,0.18f); quad(-6,-3,12,4);

    float skinR=0.85f, skinG=0.70f, skinB=0.55f;

    // Left leg
    glPushMatrix(); glTranslatef(-3,0,0); glRotatef(leg,0,0,1);
    glColor3f(student?0.12f:0.20f, student?0.22f:0.22f, student?0.62f:0.50f);
    quad(-2,-13,4,13); glPopMatrix();

    // Right leg
    glPushMatrix(); glTranslatef(3,0,0); glRotatef(-leg,0,0,1);
    glColor3f(student?0.12f:0.20f, student?0.22f:0.22f, student?0.62f:0.50f);
    quad(-2,-13,4,13); glPopMatrix();

    // Body
    glColor3f(student?0.12f:0.40f, student?0.32f:0.50f, student?0.72f:0.80f);
    glBegin(GL_QUADS);
    glVertex2f(-5,0); glVertex2f(5,0); glVertex2f(4,15); glVertex2f(-4,15);
    glEnd();

    // Left arm
    glPushMatrix(); glTranslatef(-5,11,0); glRotatef(-leg*0.8f,0,0,1);
    glColor3f(skinR,skinG,skinB); quad(-2,-11,4,11); glPopMatrix();
    // Right arm
    glPushMatrix(); glTranslatef(5,11,0); glRotatef(leg*0.8f,0,0,1);
    glColor3f(skinR,skinG,skinB); quad(-2,-11,4,11); glPopMatrix();

    // Head
    glColor3f(skinR,skinG,skinB);
    midpointCircle(0,19,6,true);
    // Hair
    glColor3f(0.18f,0.13f,0.08f);
    glBegin(GL_QUADS);
    glVertex2f(-6,20); glVertex2f(6,20); glVertex2f(5,24); glVertex2f(-5,24);
    glEnd();

    // School bag
    if(student){
        glColor3f(0.72f,0.18f,0.18f);
        quad(4,5,6,9);
    }

    glPopMatrix();
}

void drawPeople()
{
    for(auto& p: gWalkers) drawPerson(p.x,p.y,p.leg,p.student,p.rightward);
    for(auto& p: gStudents) drawPerson(p.x,p.y,p.leg,true,p.rightward);
}

// ═══════════════════════════════════════════════════════════════
//  PARK
// ═══════════════════════════════════════════════════════════════
void drawPark()
{
    float dm=gNight?0.45f:1.0f;
    // Grass patch
    glColor3f(0.24f*dm,0.65f*dm,0.24f*dm); quad(98,305,132,25);
    // Fence
    glColor3f(0.62f*dm,0.42f*dm,0.22f*dm);
    bresenhamLine(98,305,230,305);
    for(int x=100;x<=228;x+=10){ bresenhamLine(x,305,x,318); }
    // Bench
    glColor3f(0.52f*dm,0.35f*dm,0.18f*dm); quad(118,308,44,4);
    glColor3f(0.45f*dm,0.30f*dm,0.14f*dm);
    quad(120,303,4,5); quad(156,303,4,5);
    // Park trees
    drawTree(185.f,308.f); drawTree(148.f,308.f);
}

// ═══════════════════════════════════════════════════════════════
//  BILLBOARD
// ═══════════════════════════════════════════════════════════════
void drawBillboard()
{
    float dm=gNight?0.6f:1.0f;
    // Pole
    glColor3f(0.38f*dm,0.38f*dm,0.42f*dm); quad(900,305,6,68);
    // Board back
    glColor3f(0.10f*dm,0.10f*dm,0.40f*dm); quad(870,368,66,38);
    // Board text bg
    glColor3f(0.92f*dm,0.82f*dm,0.08f*dm); quad(872,370,62,34);
    glColor3f(0.05f,0.05f,0.45f);
    text(877,390,"Welcome to",GLUT_BITMAP_HELVETICA_10);
    text(876,380," Smart City!",GLUT_BITMAP_HELVETICA_10);
}

// ═══════════════════════════════════════════════════════════════
//  RAIN
// ═══════════════════════════════════════════════════════════════
void drawRain()
{
    if(!gRain) return;
    glColor4f(0.5f,0.72f,1.0f,0.55f);
    for(auto& d: gDrops)
        bresenhamLine((int)d.x,(int)d.y,(int)(d.x+3),(int)(d.y-14));
}

// ═══════════════════════════════════════════════════════════════
//  HUD
// ═══════════════════════════════════════════════════════════════
void drawHUD()
{
    float tc = gNight ? 0.92f : 0.05f;
    glColor3f(tc,tc,tc);
    text(8, H-18, gNight ? "[ NIGHT MODE ]  D=Day  N=Night  S=Anim  C=Clouds  L=Lights  P=People  V=Vehicles  R=Rain  ESC=Quit"
                         : "[ DAY MODE   ]  D=Day  N=Night  S=Anim  C=Clouds  L=Lights  P=People  V=Vehicles  R=Rain  ESC=Quit");
    char buf[160];
    sprintf(buf,"Anim:%-3s  Clouds:%-3s  Lights:%-3s  People:%-3s  Vehicles:%-3s  Rain:%-3s",
            gAnimating?"ON":"OFF", gClouds?"ON":"OFF", gLights?"ON":"OFF",
            gPeople?"ON":"OFF",    gVehicles?"ON":"OFF", gRain?"ON":"OFF");
    text(8, H-36, buf);
}

// ═══════════════════════════════════════════════════════════════
//  MASTER DISPLAY
// ═══════════════════════════════════════════════════════════════
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

    // ─── 1. Background ─────────────────────────────────────────
    drawSky();
    drawClouds();
    drawSunMoon();

    // ─── 2. Ground layer ───────────────────────────────────────
    drawGround();

    // ─── 3. Buildings ──────────────────────────────────────────
    drawBuildings();
    drawSchool();
    drawHospital();
    drawBillboard();

    // ─── 4. Road ───────────────────────────────────────────────
    drawRoad();

    // ─── 5. Street furniture ───────────────────────────────────
    drawTrees();

    // Street lights — lower sidewalk
    const float slY = 266.f;
    drawStreetLight( 76,slY); drawStreetLight(195,slY); drawStreetLight(418,slY);
    drawStreetLight(557,slY); drawStreetLight(758,slY); drawStreetLight(898,slY);
    // Street lights — upper sidewalk
    drawStreetLight( 76,110); drawStreetLight(418,110); drawStreetLight(758,110);

    // Traffic lights
    drawTrafficLight(314,150);
    drawTrafficLight(685,150);

    // ─── 6. Vehicles ───────────────────────────────────────────
    if(gVehicles) drawVehicles();

    // ─── 7. Pedestrians ────────────────────────────────────────
    if(gPeople) drawPeople();

    // ─── 8. Park ───────────────────────────────────────────────
    drawPark();

    // ─── 9. Rain / FX ──────────────────────────────────────────
    drawRain();

    // ─── 10. HUD ───────────────────────────────────────────────
    drawHUD();

    glutSwapBuffers();
}

// ═══════════════════════════════════════════════════════════════
//  TIMER / UPDATE
// ═══════════════════════════════════════════════════════════════
void timerFunc(int)
{
    if(gAnimating){
        ++gFrame;

        // Celestial bodies
        gSunX  += 0.28f; if(gSunX  > W+70) gSunX  = -70;
        gMoonX += 0.28f; if(gMoonX > W+70) gMoonX = -70;

        // Traffic lights  (~2 s per phase at 60 FPS)
        if(++gTLTimer > 118){ gTLTimer=0; gTLState=(gTLState+1)%3; }

        // Vehicles
        if(gVehicles){
            for(auto& v: gCars){
                if(v.rightward){ v.x+=v.speed; if(v.x>W+65)  v.x=-65; }
                else            { v.x-=v.speed; if(v.x<-65)   v.x=W+65; }
            }
        }

        // Pedestrians
        if(gPeople){
            float la = 26.f*sinf(gFrame*0.14f);
            for(auto& p: gWalkers){
                p.leg = la;
                if(p.rightward){ p.x+=p.speed; if(p.x>W+22) p.x=-22; }
                else            { p.x-=p.speed; if(p.x<-22)  p.x=W+22; }
            }
            float ls = 22.f*sinf(gFrame*0.11f);
            for(auto& s: gStudents){
                s.leg = ls;
                s.x += s.speed;
                // Reset past the school door (x≈400) — loop back from left
                if(s.x > 406) s.x = -40.f + (s.x-406)*0.1f;
            }
        }

        // Clouds
        if(gClouds)
            for(auto& c: gCloudList){
                c.x+=c.spd; if(c.x>W+110) c.x=-110;
            }

        // Rain
        if(gRain){
            for(auto& d: gDrops){
                d.y -= d.spd;
                if(d.y<-20){ d.y=(float)H; d.x=(float)(rand()%W); }
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);   // ~60 FPS
}

// ═══════════════════════════════════════════════════════════════
//  KEYBOARD
// ═══════════════════════════════════════════════════════════════
void keyboard(unsigned char k,int,int)
{
    switch(k){
        case 'd': case 'D': gNight=false; gSunX= 80; break;
        case 'n': case 'N': gNight=true;  gMoonX=100; break;
        case 's': case 'S': gAnimating=!gAnimating; break;
        case 'c': case 'C': gClouds=!gClouds; break;
        case 'l': case 'L': gLights=!gLights; break;
        case 'p': case 'P': gPeople=!gPeople; break;
        case 'v': case 'V': gVehicles=!gVehicles; break;
        case 'r': case 'R': gRain=!gRain; break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

// ═══════════════════════════════════════════════════════════════
//  INIT SCENE OBJECTS
// ═══════════════════════════════════════════════════════════════
void initScene()
{
    srand(7777);

    // ── Vehicles ──────────────────────────────────────────────
    // Lane 1 → rightward  y≈172
    auto addCar=[&](float x,float y,float sp,unsigned char r,unsigned char g,unsigned char b,bool rw,int tp=0){
        Vehicle v; v.x=x; v.y=y; v.speed=sp; v.r=r; v.g=g; v.b=b; v.rightward=rw; v.type=tp;
        gCars.push_back(v);
    };
    addCar( 80,172,2.0f,210, 50, 50,true);
    addCar(300,172,2.2f, 50,110,210,true);
    addCar(580,172,1.9f,220,185, 30,true);
    addCar(820,172,2.1f,180,180,185,true);
    addCar(180,177,1.3f, 50,182, 55,true,1);   // green bus
    addCar(680,178,1.2f,220,200, 50,true,1);   // yellow bus

    // Lane 2 ← leftward  y≈230
    addCar(750,232,1.8f,105, 50,155,false);
    addCar(480,232,2.0f,240,120, 30,false);
    addCar(220,232,1.7f, 50,185,185,false);
    addCar(940,232,1.9f,185, 80,110,false);
    addCar(620,228,1.1f,215, 70, 25,false,1);  // orange bus

    // ── Pedestrians ───────────────────────────────────────────
    auto addP=[&](float x,float y,float sp,bool rw,bool st=false){
        Person p; p.x=x; p.y=y; p.speed=sp; p.rightward=rw; p.student=st; p.leg=0;
        if(st) gStudents.push_back(p); else gWalkers.push_back(p);
    };
    // Lower sidewalk walkers
    addP( 45,280,0.50f,true);
    addP(210,280,0.45f,true);
    addP(490,280,0.48f,false);
    addP(790,280,0.52f,true);
    addP(960,280,0.44f,false);
    // Upper sidewalk walkers
    addP(130,126,0.46f,true);
    addP(580,126,0.50f,false);
    addP(850,126,0.43f,true);
    // Students heading right → school (x≈345)
    addP(  0,280,0.38f,true,true);
    addP( 22,280,0.38f,true,true);
    addP( 44,280,0.38f,true,true);
    addP(600,280,0.38f,true,true);
    addP(622,280,0.38f,true,true);

    // ── Clouds ────────────────────────────────────────────────
    auto addC=[&](float x,float y,float sp){
        Cloud c; c.x=x; c.y=y; c.spd=sp; gCloudList.push_back(c);
    };
    addC( 90,600,0.28f); addC(360,620,0.35f);
    addC(660,585,0.22f); addC(900,610,0.30f);

    // ── Rain drops ────────────────────────────────────────────
    for(int i=0;i<180;++i){
        Drop d;
        d.x=(float)(rand()%W);
        d.y=(float)(rand()%H);
        d.spd=5.0f+(rand()%5);
        gDrops.push_back(d);
    }
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(W, H);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("City Scene — Daffodil School & City Hospital | OpenGL");

    glClearColor(0.45f,0.78f,1.0f,1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, W, 0, H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initScene();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timerFunc, 0);

    glutMainLoop();
    return 0;
}


