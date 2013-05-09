/*
 CSCI 480
 Assignment 2
 Nishith Agarwal
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include "jpeglib.h"
#include <math.h>
#include <vector>


int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

int drag_x_origin;
int drag_y_origin;
int dragging = 0;
int zoom = 0;

float _angle = 30.0f;
float camera_angle_v = 0.0f;
float camera_angle_h = 0.0f;
float zoom_in = 0.0f;
float zoom_out = 0.0f;

float _xposition=0.0f;
float _yposition=0.0f;

float move=0.0f;
int move_i=-1;

GLuint texture;

GLfloat angle = 0.0;
float f;
int _value=0;


// represents one control point along the spline
struct point {
    double x;
    double y;
    double z;
};

// spline struct which contains how many control points, and an array of control points
struct spline {
    int numControlPoints;
    struct point *points;
};

// the spline array
struct spline *g_Splines;

// total number of splines
int g_iNumOfSplines;
point p,t,n,b,v,n1,t1,p1,b1;

void init(void)
{
    GLfloat spot[] = {0.0, 1.0, 1.0};
    GLfloat light_position[] = {0.0, 0.0, 1.0, 0.0 };
    GLfloat ambient[] = {0.184314 , 0.309804 , 0.309804, 1.0};
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);
    
    //ADDS LIGHTING EFFECTS TO THE SCENE
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot);
    glLightfv(GL_LIGHT0, GL_AMBIENT,ambient);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
}


/* Write a screenshot to the specified filename */
void saveScreenshot (int val)
{
    
    char buffer[20], buffer2[20];
    strcpy(buffer, ".jpg");
    sprintf(buffer2, "%d", val);
    char *filename = strcat(buffer2, buffer);
    
    int i,j;
    Pic *in = NULL;
    
    if (filename == NULL)
        return;
    
    in = pic_alloc(640, 480, 3, NULL);
    
    for (i=479; i>=0; i--) {
        glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                     &in->pix[i*in->nx*in->bpp]);
    }
    
    if (jpeg_write(filename, in)){
    }
    else
        printf("Error in Saving\n");
    
    pic_free(in);
    if(_value<1000)
        glutTimerFunc(66, saveScreenshot, _value++);
}


GLuint LoadTexture (const char * filename, int width, int height ){
    
    FILE *fd;
    unsigned char *image;
    int depth;
    fd = fopen(filename, "rb");
    image = (unsigned char *)malloc( width * height * 3 );
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    unsigned long location = 0;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fd);
    jpeg_read_header(&cinfo, 1);
    cinfo.scale_num = 1;
    cinfo.scale_denom = SCALE;
    
    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    height = cinfo.output_height;
    depth = cinfo.num_components;
    image = (unsigned char *) malloc(width * height * depth);
    row_pointer[0] = (unsigned char *) malloc(width * depth);
    
    while( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, row_pointer, 1 );
        for(int i=0; i< (width * depth); i++)
            image[location++] = row_pointer[0][i];
    }
    fclose(fd);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    //glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    free(image);
    
    return texture;
    
}

void mousedrag(int x, int y)
{
    int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
    
    
    switch (g_ControlState)
    {
        case TRANSLATE:
            if (g_iLeftMouseButton)
            {
                g_vLandTranslate[0] += vMouseDelta[0]*0.001;
                g_vLandTranslate[1] -= vMouseDelta[1]*0.001;
                
                if (abs(x - (int)drag_x_origin) > 20) {
                    _yposition = x-drag_x_origin-0.1;
                }
                
                else if (abs(y - (int)drag_y_origin) > 20){
                    _xposition = y-drag_y_origin-0.1;
                }
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandTranslate[2] += vMouseDelta[1]*0.01;
            }
            break;
        case ROTATE:
            if (g_iLeftMouseButton)
            {
                g_vLandRotate[0] += vMouseDelta[1];
                g_vLandRotate[1] += vMouseDelta[0];
                
                camera_angle_v += (y - drag_y_origin)*0.3;
                camera_angle_h += (x - drag_x_origin)*0.3;
                drag_x_origin = x;
                drag_y_origin = y;
                
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandRotate[2] += vMouseDelta[1];
                
                camera_angle_v -= (y - drag_y_origin)*0.3;
                camera_angle_h -= (x - drag_x_origin)*0.3;
                drag_x_origin = x;
                drag_y_origin = y;
                
            }
            break;
        case SCALE:
            if (g_iLeftMouseButton)
            {
                g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
                g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
                zoom_in +=0.1;
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
                zoom_in -=0.1;
                
                
                
            }
            break;
    }
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
    
}

void mousebutton(int button, int state, int x, int y)
{
    
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            g_iLeftMouseButton = (state==GLUT_DOWN);
            dragging=1;
            drag_x_origin = x;
            drag_y_origin = y;
            break;
        case GLUT_MIDDLE_BUTTON:
            g_iMiddleMouseButton = (state==GLUT_DOWN);
            break;
        case GLUT_RIGHT_BUTTON:
            g_iRightMouseButton = (state==GLUT_DOWN);
            break;
            
    }
    
    switch(glutGetModifiers())
    {
        case GLUT_ACTIVE_CTRL:
            g_ControlState = TRANSLATE;
            break;
        case GLUT_ACTIVE_SHIFT:
            g_ControlState = SCALE;
            break;
        default:
            g_ControlState = ROTATE;
            break;
    }
    
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void doIdle()
{
    glutPostRedisplay();
}


int loadSplines(char *argv) {
    char *cName = (char *)malloc(128 * sizeof(char));
    FILE *fileList;
    FILE *fileSpline;
    int iType, i = 0, j, iLength;
    
    
    // load the track file
    fileList = fopen(argv, "r");
    if (fileList == NULL) {
        printf ("can't open file\n");
        exit(1);
    }
    
    // stores the number of splines in a global variable
    fscanf(fileList, "%d", &g_iNumOfSplines);
    
    g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));
    
    // reads through the spline files
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL) {
            printf ("can't open file\n");
            exit(1);
        }
        
        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        // allocate memory for all the points
        g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
        g_Splines[j].numControlPoints = iLength;
        
        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &g_Splines[j].points[i].x,
                      &g_Splines[j].points[i].y,
                      &g_Splines[j].points[i].z) != EOF) {
            i++;
        }
    }
    
    free(cName);
    
    return 0;
}



float CatmullRom( float t, float p1, float p2, float p3, float p4 )
{
    return 0.5 * (
                  (-p1 + 3*p2 -3*p3 + p4)*t*t*t
                  + (2*p1 -5*p2 + 4*p3 - p4)*t*t
                  + (-p1+p3)*t
                  + (2*p2)
                  );
}

float CatmullRomTangent( float t, float p1, float p2, float p3, float p4 )
{
    
    float m = 0.5 * (
                     (-p1 + 3*p2 -3*p3 + p4)*3*t*t
                     + (2*p1 -5*p2 + 4*p3 - p4)*2*t
                     + (-p1+p3)
                     );
    
    return m;
}

point CrossProduct( point p1, point p2)
{
    point p3;
    /*
     i       j       k
     p1.x    p1.y    p1.z
     p2.x    p2.y    p2.z
     */
    
    p3.x = (p1.y*p2.z)-(p2.y*p1.z);
    p3.y = (p2.x*p1.z)-(p1.x*p2.z);
    p3.z = (p1.x*p2.y)-(p2.x*p1.y);
    
    return p3;
    
    
}

point normalize(point p3)
{
    float length_n = sqrt((p3.x*p3.x)+(p3.y*p3.y)+(p3.z*p3.z));
    p3.x=(float) p3.x/length_n;
    p3.y=(float) p3.y/length_n;
    p3.z=(float) p3.z/length_n;
    
    return p3;
}

//DISPLAY THE BACKGROUND SCENE
void displayBackground()
{
    //DISPLAY THE SKY
    texture = LoadTexture("sky1.jpg",256, 256);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    //xz plane
    glTexCoord2d(0.0,0.0); glVertex3d(-100,-100.0,-100);
    glTexCoord2d(1.0,0.0); glVertex3d(100.0,-100.0,-100);
    glTexCoord2d(1.0,1.0); glVertex3d(100.0,-100.0,100);
    glTexCoord2d(0.0,1.0); glVertex3d(-100.0,-100.0,100);
    //zy plane
    glTexCoord2d(0.0,0.0); glVertex3d(-100,-100,-100);
    glTexCoord2d(1.0,0.0); glVertex3d(-100,100.0,-100);
    glTexCoord2d(1.0,1.0); glVertex3d(-100,100.0,100);
    glTexCoord2d(0.0,1.0); glVertex3d(-100,-100.0,100);
    //xz plane
    glTexCoord2d(0.0,0.0); glVertex3d(-100,100.0,-100);
    glTexCoord2d(1.0,0.0); glVertex3d(100.0,100.0,-100);
    glTexCoord2d(1.0,1.0); glVertex3d(100.0,100.0,100);
    glTexCoord2d(0.0,1.0); glVertex3d(-100.0,100.0,100);
    //zy plane
    glTexCoord2d(0.0,0.0); glVertex3d(100,-100,-100);
    glTexCoord2d(1.0,0.0); glVertex3d(100,100.0,-100);
    glTexCoord2d(1.0,1.0); glVertex3d(100,100.0,100);
    glTexCoord2d(0.0,1.0); glVertex3d(100,-100.0,100);
    //xy plane
    glTexCoord2d(0.0,0.0); glVertex3d(-100,-100.0,100);
    glTexCoord2d(1.0,0.0); glVertex3d(100.0,-100.0,100);
    glTexCoord2d(1.0,1.0); glVertex3d(100.0,100.0,100);
    glTexCoord2d(0.0,1.0); glVertex3d(-100.0,100.0,100);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    //DISPLAY THE GROUND
    // glColor3f(1.0, 0.5, 0.4);
    texture = LoadTexture("wood.jpg",256, 256);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0,0.0); glVertex3d(-100,-100.0,-2);
    glTexCoord2d(200.0,0.0); glVertex3d(100.0,-100.0,-2);
    glTexCoord2d(200.0,200.0); glVertex3d(100.0,100.0,-2);
    glTexCoord2d(0.0,200.0); glVertex3d(-100.0,100.0,-2);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);
}

//DISPLAY ROLLER COASTER COLUMNS
void displayColumns()
{
    float a=0.004,w=0.02;
     glColor3f(0.0, 1.0, 0.0);
    texture = LoadTexture("wood.jpg",256, 256);
    glEnable(GL_TEXTURE_2D);
    for(float r=0;r<2;r++){
        glBegin(GL_QUADS);
        for (int j = 0; j < g_iNumOfSplines; j++) {
            for(int i=-2;i<g_Splines[j].numControlPoints-2;i++)
            {
                for(float u=0.0;u<1.0;u+=0.8){
                    //Point
                    p.x= CatmullRom(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p.y= CatmullRom(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p.z= CatmullRom(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    //Tangent
                    t.x= CatmullRomTangent(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t.y= CatmullRomTangent(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t.z= CatmullRomTangent(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t = normalize(t);
                    //Normal
                    n = CrossProduct(t, v);
                    n = normalize(n);
                    //Binormal
                    b = CrossProduct(t, n);
                    b = normalize(b);
                    
                    p1.x= CatmullRom(u+w, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p1.y= CatmullRom(u+w, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p1.z= CatmullRom(u+w, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    
                    t1.x= CatmullRomTangent(u+w, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t1.y= CatmullRomTangent(u+w, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t1.z= CatmullRomTangent(u+w, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t1 = normalize(t);
                    
                    n1 = CrossProduct(t1, v);
                    n1 = normalize(n1);
                    b1 = CrossProduct(t1, n1);
                    b1 = normalize(b1);
                    
                    if(r==1)
                    {
                        p.x+=f*n.x;
                        p.y+=f*n.y;
                        p.z+=f*n.z;
                        
                        p1.x+=f*n1.x;
                        p1.y+=f*n1.y;
                        p1.z+=f*n1.z;
                        
                    }
                    
                    // glColor3f(0.0, 0.7, 1.0);
                    //V1
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V5
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                    //V5
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),-5);
                    //V1
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),-5);
                    
                    //V1
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V4
                    glTexCoord2d(1.0,0.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    //V4
                    glTexCoord2d(1.0,1.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),-5);
                    //V1
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),-5);
                    
                    //V4
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    //V8
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    //V8
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),-5);
                    //V4
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),-5);
                    
                    //glColor3f(0.2, 0.9, 0.6);
                    //V5
                    glTexCoord2d(0.0,0.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                    //V8
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    //V8
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),-5);
                    //V5
                    glTexCoord2d(0.0,1.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),-5);
                    
                }
            }
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    
}

//DISPLAY THE ROLLER COASTER
void displayRollerCoaster()
{
    float a=0.003,h=0.04;
    glColor3f(0.2,0.2,1.0);
    texture = LoadTexture("steel.jpg",256, 256);
    glEnable(GL_TEXTURE_2D);
    for(int r=0;r<4;r++){
        glBegin(GL_QUADS);
        for (int j = 0; j < g_iNumOfSplines; j++) {
            for(int i=-2;i<g_Splines[j].numControlPoints-1;i++)
            { //printf("%f %f %f\n",g_Splines[j].points[i].x,g_Splines[j].points[i].y,g_Splines[j].points[i].z);
                for(float u=0.0;u<1.0;u+=0.02){
                    //Point
                    p.x= CatmullRom(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p.y= CatmullRom(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p.z= CatmullRom(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    //Tangent
                    t.x= CatmullRomTangent(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t.y= CatmullRomTangent(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t.z= CatmullRomTangent(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t = normalize(t);
                    //Normal
                    n = CrossProduct(t, v);
                    n = normalize(n);
                    //Binormal
                    b = CrossProduct(t, n);
                    b = normalize(b);
                    
                    p1.x= CatmullRom(u+0.02, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p1.y= CatmullRom(u+0.02, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p1.z= CatmullRom(u+0.02, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    
                    t1.x= CatmullRomTangent(u+0.02, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t1.y= CatmullRomTangent(u+0.02, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t1.z= CatmullRomTangent(u+0.02, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t1 = normalize(t1);
                    
                    n1 = CrossProduct(t1, v);
                    n1 = normalize(n1);
                    b1 = CrossProduct(t1, n1);
                    b1 = normalize(b1);
                    
                    if(r==1)
                    {
                        p.x+=f*n.x;
                        p.y+=f*n.y;
                        p.z+=f*n.z;
                        
                        p1.x+=f*n1.x;
                        p1.y+=f*n1.y;
                        p1.z+=f*n1.z;
                        
                    }
                    
                    if(r==2)
                    {
                        p.x+=h*b.x;
                        p.y+=h*b.y;
                        p.z+=h*b.z;
                        
                        p1.x+=h*b1.x;
                        p1.y+=h*b1.y;
                        p1.z+=h*b1.z;
                        
                    }
                    
                    if(r==3)
                    {
                        p.x+=f*n.x+ h*b.x;
                        p.y+=f*n.y+ h*b.y;
                        p.z+=f*n.z+ h*b.z;
                        
                        p1.x+=f*n1.x+h*b1.x;
                        p1.y+=f*n1.y+h*b1.y;
                        p1.z+=f*n1.z+h*b1.z;
                        
                    }
                    
                    //glVertex3f(p.x, p.y, p.z);
                    
                    //V1
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V2
                    glTexCoord2d(1.0,0.0);glVertex3f(p.x+a*(n.x+b.x),p.y+a*(n.y+b.y),p.z+a*(n.z+b.z));
                    //V3
                    glTexCoord2d(1.0,1.0);glVertex3f(p.x+a*(-n.x+b.x),p.y+a*(-n.y+b.y),p.z+a*(-n.z+b.z));
                    //V4
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    
                    //V5
                    glTexCoord2d(0.0,0.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                    //V6
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(n1.x+b1.x),p1.y+a*(n1.y+b1.y),p1.z+a*(n1.z+b1.z));
                    //V7
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(-n1.x+b1.x),p1.y+a*(-n1.y+b1.y),p1.z+a*(-n1.z+b1.z));
                    //V8
                    glTexCoord2d(0.0,1.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    
                    //V1
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V4
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    //V8
                    glTexCoord2d(0.0,1.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    //V5
                    glTexCoord2d(0.0,0.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                    
                    //V3
                    glTexCoord2d(1.0,1.0);glVertex3f(p.x+a*(-n.x+b.x),p.y+a*(-n.y+b.y),p.z+a*(-n.z+b.z));
                    //V4
                    glTexCoord2d(0.0,1.0);glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    //V8
                    glTexCoord2d(0.0,1.0);glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    //V7
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(-n1.x+b1.x),p1.y+a*(-n1.y+b1.y),p1.z+a*(-n1.z+b1.z));
                    
                    //V2
                    glTexCoord2d(1.0,0.0);glVertex3f(p.x+a*(n.x+b.x),p.y+a*(n.y+b.y),p.z+a*(n.z+b.z));
                    //V3
                    glTexCoord2d(1.0,1.0);glVertex3f(p.x+a*(-n.x+b.x),p.y+a*(-n.y+b.y),p.z+a*(-n.z+b.z));
                    //V7
                    glTexCoord2d(1.0,1.0);glVertex3f(p1.x+a*(-n1.x+b1.x),p1.y+a*(-n1.y+b1.y),p1.z+a*(-n1.z+b1.z));
                    //V6
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(n1.x+b1.x),p1.y+a*(n1.y+b1.y),p1.z+a*(n1.z+b1.z));
                    
                    //V1
                    glTexCoord2d(0.0,0.0);glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V2
                    glTexCoord2d(1.0,0.0);glVertex3f(p.x+a*(n.x+b.x),p.y+a*(n.y+b.y),p.z+a*(n.z+b.z));
                    //V6
                    glTexCoord2d(1.0,0.0);glVertex3f(p1.x+a*(n1.x+b1.x),p1.y+a*(n1.y+b1.y),p1.z+a*(n1.z+b1.z));
                    //V5
                    glTexCoord2d(0.0,0.0);glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                }
            }
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);
}

//DISPLAY MINI RAILS ON THE ROLLER COASTER
void miniRails()
{
    point p2,p3;
    float a=0.0015,h=0.04;
    for(float r=0;r<2;r++){
        glBegin(GL_QUADS);
        for (int j = 0; j < g_iNumOfSplines; j++) {
            for(int i=-2;i<g_Splines[j].numControlPoints-1;i++)
            {
                for(float u=0.0;u<1.0;u+=0.05){
                    //Point
                    p.x= CatmullRom(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p.y= CatmullRom(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p.z= CatmullRom(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    //Tangent
                    t.x= CatmullRomTangent(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t.y= CatmullRomTangent(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t.z= CatmullRomTangent(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t = normalize(t);
                    //Normal
                    n = CrossProduct(t, v);
                    n = normalize(n);
                    //Binormal
                    b = CrossProduct(t, n);
                    b = normalize(b);
                    
                    p1.x= CatmullRom(u+0.005, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    p1.y= CatmullRom(u+0.005, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    p1.z= CatmullRom(u+0.005, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    
                    t1.x= CatmullRomTangent(u+0.005, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                    t1.y= CatmullRomTangent(u+0.005, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                    t1.z= CatmullRomTangent(u+0.005, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                    t1 = normalize(t1);
                    
                    n1 = CrossProduct(t1, v);
                    n1 = normalize(n1);
                    b1 = CrossProduct(t1, n1);
                    b1 = normalize(b1);
                    
                    if(r==1)
                    {
                        p.x+=f*n.x;
                        p.y+=f*n.y;
                        p.z+=f*n.z;
                        
                        p1.x+=f*n1.x;
                        p1.y+=f*n1.y;
                        p1.z+=f*n1.z;
                        
                    }
                    
                    p2.x=p.x+h*b.x;
                    p2.y=p.y+h*b.y;
                    p2.z=p.z+h*b.z;
                    
                    p3.x = p1.x+h*b1.x;
                    p3.y = p1.y+h*b1.y;
                    p3.z = p1.z+h*b1.z;
                    
                    glColor3f(0.4, 0.0, 0.0);
                    //V5
                    glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                    //V8
                    glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                    //V8
                    glVertex3f(p3.x+a*(-n1.x-b1.x),p3.y+a*(-n1.y-b1.y),p3.z+a*(-n1.z-b1.z));
                    //V5
                    glVertex3f(p3.x+a*(n1.x-b1.x),p3.y+a*(n1.y-b1.y),p3.z+a*(n1.z-b1.z));
                    
                    if(r==0){
                        glColor3f(1.0, 0.0, 0.0);
                        //V4
                        glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                        //V8
                        glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                        //V8
                        glVertex3f(p3.x+a*(-n1.x-b1.x),p3.y+a*(-n1.y-b1.y),p3.z+a*(-n1.z-b1.z));
                        //V4
                        glVertex3f(p2.x+a*(-n.x-b.x),p2.y+a*(-n.y-b.y),p2.z+a*(-n.z-b.z));
                        
                        //V1
                        glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                        //V5
                        glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                        //V5
                        glVertex3f(p3.x+a*(n1.x-b1.x),p3.y+a*(n1.y-b1.y),p3.z+a*(n1.z-b1.z));
                        //V1
                        glVertex3f(p2.x+a*(n.x-b.x),p2.y+a*(n.y-b.y),p2.z+a*(n.z-b.z));
                    }
                    else{
                        glColor3f(1.0, 0.0, 0.0);
                        //V1
                        glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                        //V5
                        glVertex3f(p1.x+a*(n1.x-b1.x),p1.y+a*(n1.y-b1.y),p1.z+a*(n1.z-b1.z));
                        //V5
                        glVertex3f(p3.x+a*(n1.x-b1.x),p3.y+a*(n1.y-b1.y),p3.z+a*(n1.z-b1.z));
                        //V1
                        glVertex3f(p2.x+a*(n.x-b.x),p2.y+a*(n.y-b.y),p2.z+a*(n.z-b.z));
                        
                        //V4
                        glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                        //V8
                        glVertex3f(p1.x+a*(-n1.x-b1.x),p1.y+a*(-n1.y-b1.y),p1.z+a*(-n1.z-b1.z));
                        //V8
                        glVertex3f(p3.x+a*(-n1.x-b1.x),p3.y+a*(-n1.y-b1.y),p3.z+a*(-n1.z-b1.z));
                        //V4
                        glVertex3f(p2.x+a*(-n.x-b.x),p2.y+a*(-n.y-b.y),p2.z+a*(-n.z-b.z));
                    }
                    
                    
                    
                    glColor3f(0.4, 0.0, 0.0);
                    //V1
                    glVertex3f(p.x+a*(n.x-b.x),p.y+a*(n.y-b.y),p.z+a*(n.z-b.z));
                    //V4
                    glVertex3f(p.x+a*(-n.x-b.x),p.y+a*(-n.y-b.y),p.z+a*(-n.z-b.z));
                    //V4
                    glVertex3f(p2.x+a*(-n.x-b.x),p2.y+a*(-n.y-b.y),p2.z+a*(-n.z-b.z));
                    //V1
                    glVertex3f(p2.x+a*(n.x-b.x),p2.y+a*(n.y-b.y),p2.z+a*(n.z-b.z));
                    
                    
                }
            }
        }
        glEnd();
    }
    glColor3f(1.0, 1.0, 1.0);
}

//DISPLAY RAILS ON ROLLER COASTER
void displayRails()
{
    texture = LoadTexture("steel.jpg",256, 256);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    for (int j = 0; j < g_iNumOfSplines; j++) {
        for(int i=g_Splines[j].numControlPoints-2;i>=-2;i--)
        {
            for(float u=0.0;u<1.0;u+=0.05){
                
                //Point
                p.x= CatmullRom(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                p.y= CatmullRom(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                p.z= CatmullRom(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                //Tangent
                t.x= CatmullRomTangent(u, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                t.y= CatmullRomTangent(u, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                t.z= CatmullRomTangent(u, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                t = normalize(t);
                //Normal
                n = CrossProduct(t, v);
                n = normalize(n);
                
                p1.x= CatmullRom(u+0.01, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                p1.y= CatmullRom(u+0.01, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                p1.z= CatmullRom(u+0.01, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                
                t1.x= CatmullRomTangent(u+0.01, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
                t1.y= CatmullRomTangent(u+0.01, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
                t1.z= CatmullRomTangent(u+0.01, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
                t1 = normalize(t1);
                
                n1 = CrossProduct(t1, v);
                n1 = normalize(n1);
                
                glColor3f(1.0, 1.0, 1.0);
                glTexCoord2d(0.0,0.0);glVertex3f(p.x,p.y,p.z);
                glTexCoord2d(3,0.0);glVertex3f(p.x+f*(n.x),p.y+f*(n.y),p.z+f*(n.z));
                glTexCoord2d(3,1);glVertex3f(p1.x+f*(n1.x),p1.y+f*(n1.y),p1.z+f*(n1.z));
                glTexCoord2d(0.0,1);glVertex3f(p1.x,p1.y,p1.z);
                
                glColor3f(1.0, 1.0, 1.0);
                glTexCoord2d(0.0,0.0);glVertex3f(p.x,p.y,p.z-0.006);
                glTexCoord2d(3.0,0.0);glVertex3f(p.x+f*(n.x),p.y+f*(n.y),p.z+f*(n.z)-0.006);
                glTexCoord2d(3.0,1.0);glVertex3f(p1.x+f*(n1.x),p1.y+f*(n1.y),p1.z+f*(n1.z)-0.006);
                glTexCoord2d(0.0,1.0);glVertex3f(p1.x,p1.y,p1.z-0.006);
                
                glColor3f(0.6, 0.6, 0.6);
                glTexCoord2d(0.0,0.0);glVertex3f(p.x,p.y,p.z);
                glTexCoord2d(3.0,0.0);glVertex3f(p.x+f*(n.x),p.y+f*(n.y),p.z+f*(n.z));
                glTexCoord2d(3.0,1.0);glVertex3f(p.x+f*(n.x),p.y+f*(n.y),p.z+f*(n.z)-0.006);
                glTexCoord2d(0.0,1.0);glVertex3f(p.x,p.y,p.z-0.006);
                
                glColor3f(1.0, 1.0, 1.0);
                glTexCoord2d(0.0,0.0);glVertex3f(p1.x,p1.y,p1.z);
                glTexCoord2d(3.0,0.0);glVertex3f(p1.x+f*(n1.x),p1.y+f*(n1.y),p1.z+f*(n1.z));
                glTexCoord2d(3.0,1.0);glVertex3f(p1.x+f*(n1.x),p1.y+f*(n1.y),p1.z+f*(n1.z)-0.006);
                glTexCoord2d(0.0,1.0);glVertex3f(p1.x,p1.y,p1.z-0.006);
                
                
            }
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);
}

//MOVE THE CAMERA ON THE ROLLER COASTER
void camera()
{
    int j=0;
    int i=move_i;
    
    p.x= CatmullRom(move, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
    p.y= CatmullRom(move, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
    p.z= CatmullRom(move, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
    
    t.x= CatmullRomTangent(move, g_Splines[j].points[i].x, g_Splines[j].points[i+1].x, g_Splines[j].points[i+2].x, g_Splines[j].points[i+3].x);
    t.y= CatmullRomTangent(move, g_Splines[j].points[i].y, g_Splines[j].points[i+1].y, g_Splines[j].points[i+2].y, g_Splines[j].points[i+3].y);
    t.z= CatmullRomTangent(move, g_Splines[j].points[i].z, g_Splines[j].points[i+1].z, g_Splines[j].points[i+2].z, g_Splines[j].points[i+3].z);
    t = normalize(t);
    
    n = CrossProduct(t, v);
    n = normalize(n);
    
    b = CrossProduct(t, n);
    b = normalize(b);
    
    point eye;
    
    float h=0.1;
    eye.x = p.x + (f/2.0)*n.x + h*b.x;
    eye.y = p.y + (f/2.0)*n.y + h*b.y;
    eye.z = p.z + (f/2.0)*n.z + h*b.z;
    
    
    
    gluLookAt(
              // 5.0,5.0,14.0,
              eye.x,eye.y,eye.z,
              p.x+(f/2.0)*n.x+t.x, p.y+(f/2.0)*n.y+t.y, p.z+(f/2.0)*n.z+t.z,
              
              b.x,b.y,b.z);
}

void display(void)
{
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    
    glPushMatrix();
    
    //TRANSLATION - Move object on X or Y axis using mouse drag + CTRL key
    glTranslatef(_yposition, _xposition,0.0f);
    
    //ZOOM - Zoom in or zoom out using mouse drag + SHIFT key
    glScalef(0.5f+zoom_in,0.5f+zoom_in,0.5f+zoom_in);
    
    //ROTATION - Rotate image along X and Y plane using mouse drag
    glRotated(camera_angle_v, 1.0, 0.0, 0.0);
    glRotated(camera_angle_h, 0.0, 1.0, 0.0);
    
    f=0.08;
    v.x=0.0;v.y=0.0;v.z=-1.0;
    
    camera();
    displayBackground();
    displayColumns();
    miniRails();
    displayRails();
    displayRollerCoaster();

    glPopMatrix();
    glutSwapBuffers();
}


void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    
    glLoadIdentity();
    gluPerspective(20.0, (GLfloat)w/(GLfloat)h, 0.01, 1000.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void handleKeypress(unsigned char key, int x, int y)
{
    switch(key) {
        case 27: exit(0);
            
        case 'm': move+=0.02;
            if(move>=1.0){
                move_i++;
                move=0.0;
                if(move_i>=g_Splines[0].numControlPoints-3)
                    move_i=0;
            }
            break;
        case 'a': glEnable(GL_LIGHT0);
            break;
        case 's':
            glDisable(GL_LIGHT0);
            break;
            
    }
    
}

int main (int argc, char ** argv)
{
    if (argc<2)
    {
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    
    loadSplines(argv[1]);
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (640, 480);
    glutInitWindowPosition (100, 100);
    glutCreateWindow (argv[0]);
    init ();
    glClearDepth(1.0);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    
    glutKeyboardFunc(handleKeypress);
    glutIdleFunc(doIdle);
    glutMotionFunc(mousedrag);
    glutPassiveMotionFunc(mouseidle);
    glutMouseFunc(mousebutton);
    //saveScreenshot(0);
    glutMainLoop();
    return 0;
    
}
