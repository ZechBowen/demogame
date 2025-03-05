/*____________________________________________________________________
|
| File: main.cpp
|
| Description: Main module in program.
|
| Functions:  Program_Get_User_Preferences
|             Program_Init
|							 Init_Graphics
|								Set_Mouse_Cursor
|             Program_Run
|							 Init_Render_State
|             Program_Free
|             Program_Immediate_Key_Handler               
|
| (C) Copyright 2013 Abonvita Software LLC.
| Licensed under the GX Toolkit License, Version 1.0.
|___________________________________________________________________*/

#define _MAIN_

/*___________________
|
| Include Files
|__________________*/

#include <first_header.h>
#include "dp.h"
#include "..\Framework\win_support.h"
#include <rom8x8.h>

#include "main.h"
#include "position.h"

/*___________________
|
| Type definitions
|__________________*/

typedef struct {
  unsigned resolution;
  unsigned bitdepth;
} UserPreferences;

/*___________________
|
| Function Prototypes
|__________________*/

static int Init_Graphics (unsigned resolution, unsigned bitdepth, unsigned stencildepth, int *generate_keypress_events);
static void Set_Mouse_Cursor ();
static void Init_Render_State ();

/*___________________
|
| Constants
|__________________*/

#define MAX_VRAM_PAGES  2
#define GRAPHICS_RESOLUTION  \
  (                          \
    gxRESOLUTION_640x480   | \
    gxRESOLUTION_800x600   | \
    gxRESOLUTION_1024x768  | \
    gxRESOLUTION_1152x864  | \
    gxRESOLUTION_1280x960  | \
    gxRESOLUTION_1400x1050 | \
    gxRESOLUTION_1440x1080 | \
    gxRESOLUTION_1600x1200 | \
    gxRESOLUTION_1152x720  | \
    gxRESOLUTION_1280x800  | \
    gxRESOLUTION_1440x900  | \
    gxRESOLUTION_1680x1050 | \
    gxRESOLUTION_1920x1200 | \
    gxRESOLUTION_2048x1280 | \
    gxRESOLUTION_1280x720  | \
    gxRESOLUTION_1600x900  | \
    gxRESOLUTION_1920x1080   \
  )
#define GRAPHICS_STENCILDEPTH 0
#define GRAPHICS_BITDEPTH (gxBITDEPTH_24 | gxBITDEPTH_32)

#define AUTO_TRACKING    1
#define NO_AUTO_TRACKING 0

/*____________________________________________________________________
|
| Function: Program_Get_User_Preferences
|
| Input: Called from CMainFrame::Init
| Output: Allows program to popup dialog boxes, etc. to get any user
|   preferences such as screen resolution.  Returns preferences via a
|   pointer.  Returns true on success, else false to quit the program.
|___________________________________________________________________*/

int Program_Get_User_Preferences (void **preferences)
{
  static UserPreferences user_preferences;

  if (gxGetUserFormat (GRAPHICS_DRIVER, GRAPHICS_RESOLUTION, GRAPHICS_BITDEPTH, &user_preferences.resolution, &user_preferences.bitdepth)) {
    *preferences = (void *)&user_preferences;
    return (1);
  }
  else
    return (0);
}

/*____________________________________________________________________
|
| Function: Program_Init
|
| Input: Called from CMainFrame::Start_Program_Thread()
| Output: Starts graphics mode.  Returns # of user pages available if 
|       successful, else 0.
|___________________________________________________________________*/
 
int Program_Init (void *preferences, int *generate_keypress_events)
{
  UserPreferences *user_preferences = (UserPreferences *) preferences;
  int initialized = FALSE;

  if (user_preferences) 
    initialized = Init_Graphics (user_preferences->resolution, user_preferences->bitdepth, GRAPHICS_STENCILDEPTH, generate_keypress_events);
    
  return (initialized);
}

/*____________________________________________________________________
|
| Function: Init_Graphics
|
| Input: Called from Program_Init()
| Output: Starts graphics mode.  Returns # of user pages available if 
|       successful, else 0.
|___________________________________________________________________*/

static int Init_Graphics (unsigned resolution, unsigned bitdepth, unsigned stencildepth, int *generate_keypress_events)
{
  int num_pages;
  byte *font_data;
  unsigned font_size;

/*____________________________________________________________________
|
| Init globals
|___________________________________________________________________*/

  Pgm_num_pages   = 0;
  Pgm_system_font = NULL;

/*____________________________________________________________________
|
| Start graphics mode and event processing
|___________________________________________________________________*/

  font_data = font_data_rom8x8;
  font_size = sizeof(font_data_rom8x8);
                                                                      
  // Start graphics mode                                      
  num_pages = gxStartGraphics (resolution, bitdepth, stencildepth, MAX_VRAM_PAGES, GRAPHICS_DRIVER);
  if (num_pages == MAX_VRAM_PAGES) {
    // Init system, drawing fonts 
    Pgm_system_font  = gxLoadFontData (gxFONT_TYPE_GX, font_data, font_size);
    // Make system font the default drawing font 
    gxSetFont (Pgm_system_font);

    // Start event processing
    evStartEvents (evTYPE_MOUSE_LEFT_PRESS     | evTYPE_MOUSE_RIGHT_PRESS   |
                   evTYPE_MOUSE_LEFT_RELEASE   | evTYPE_MOUSE_RIGHT_RELEASE |
                   evTYPE_MOUSE_WHEEL_BACKWARD | evTYPE_MOUSE_WHEEL_FORWARD |
//                   evTYPE_KEY_PRESS | 
                   evTYPE_RAW_KEY_PRESS | evTYPE_RAW_KEY_RELEASE,
                   AUTO_TRACKING, EVENT_DRIVER);                              
    *generate_keypress_events = FALSE;  // true if using evTYPE_KEY_PRESS in the above mask

		// Set a custom mouse cursor
		Set_Mouse_Cursor ();

    // Set globals
    Pgm_num_pages = num_pages;
  }

  return (Pgm_num_pages);
}

/*____________________________________________________________________
|
| Function: Set_Mouse_Cursor
|
| Input: Called from Init_Graphics()
| Output: Sets default mouse cursor.
|___________________________________________________________________*/
     
static void Set_Mouse_Cursor ()
{
  gxColor fc, bc;

  // Set cursor to a medium sized red arrow
  fc.r = 255;
  fc.g = 0;
  fc.b = 0;
  fc.a = 0;
  bc.r = 1;
  bc.g = 1;
  bc.b = 1;
  bc.a = 0;
  msSetCursor (msCURSOR_MEDIUM_ARROW, fc, bc);
}

/*____________________________________________________________________
|
| Function: Program_Run
|
| Input: Called from Program_Thread()
| Output: Runs program in the current video mode.  Begins with mouse
|   hidden.
|___________________________________________________________________*/
                    
void Program_Run ()
{
  int quit;
  evEvent event;
  gx3dDriverInfo dinfo;
  gxColor color;
  char str[256];

  gx3dObject *obj_tree, *obj_tree2, *obj_skydome, *obj_clouddome, *obj_ghost;
  gx3dMatrix m, m1, m2, m3, m4, m5;
  gx3dColor color3d_white    = { 1, 1, 1, 0 };
  gx3dColor color3d_dim      = { 0.1f, 0.1f, 0.1f };
  gx3dColor color3d_black    = { 0, 0, 0, 0 };
  gx3dColor color3d_darkgray = { 0.3f, 0.3f, 0.3f, 0 };
  gx3dColor color3d_gray     = { 0.5f, 0.5f, 0.5f, 0 };
  gx3dMaterialData material_default = {
    { 1, 1, 1, 1 }, // ambient color
    { 1, 1, 1, 1 }, // diffuse color
    { 1, 1, 1, 1 }, // specular color
    { 0, 0, 0, 0 }, // emissive color
    10              // specular sharpness (0=disabled, 0.01=sharp, 10=diffused)
  };

 // // How to use C++ print outupt 
 // string mystr;

	//mystr = "SUPER!";
	//char ss[256];
	//strcpy (ss, mystr.c_str());
	//debug_WriteFile (ss);

/*____________________________________________________________________
|
| Print info about graphics driver to debug file.
|___________________________________________________________________*/

  gx3d_GetDriverInfo (&dinfo);
  debug_WriteFile ("_______________ Device Info ______________");
  sprintf (str, "max texture size: %dx%d", dinfo.max_texture_dx, dinfo.max_texture_dy);
  debug_WriteFile (str);
  sprintf (str, "max active lights: %d", dinfo.max_active_lights);
  debug_WriteFile (str);
  sprintf (str, "max user clip planes: %d", dinfo.max_user_clip_planes);
  debug_WriteFile (str);
  sprintf (str, "max simultaneous texture stages: %d", dinfo.max_simultaneous_texture_stages);
  debug_WriteFile (str);
  sprintf (str, "max texture stages: %d", dinfo.max_texture_stages);
  debug_WriteFile (str);
  sprintf (str, "max texture repeat: %d", dinfo.max_texture_repeat);
  debug_WriteFile (str);
  debug_WriteFile ("__________________________________________");

/*____________________________________________________________________
|
| Initialize the sound library 
|___________________________________________________________________*/

  snd_Init(22, 16, 2, 1, 1);
  snd_SetListenerDistanceFactorToFeet(snd_3D_APPLY_NOW);

  Sound s_steps, s_song, s_ring, s_pew, s_crash;

  s_song = snd_LoadSound("wav\\theme_song.wav", snd_CONTROL_VOLUME, 0);
  s_steps = snd_LoadSound("wav\\footsteps.wav", snd_CONTROL_VOLUME, 0);
  s_ring = snd_LoadSound("wav\\ring.wav", snd_CONTROL_3D, 0);
  s_pew = snd_LoadSound("wav\\pew.wav", snd_CONTROL_VOLUME, 0);
  s_crash = snd_LoadSound("wav\\crash.wav", snd_CONTROL_VOLUME, 0);


/*____________________________________________________________________
|
| Initialize the graphics state
|___________________________________________________________________*/

  // Set 2d graphics state
  Pgm_screen.xleft   = 0;
  Pgm_screen.ytop    = 0;
  Pgm_screen.xright  = gxGetScreenWidth() - 1;
  Pgm_screen.ybottom = gxGetScreenHeight() - 1;
  gxSetWindow (&Pgm_screen);
  gxSetClip   (&Pgm_screen);
  gxSetClipping (FALSE);

	// Set the 3D viewport
	gx3d_SetViewport (&Pgm_screen);
	// Init other 3D stuff
	Init_Render_State ();

/*____________________________________________________________________
|
| Init support routines
|___________________________________________________________________*/

	gx3dVector heading, position;

	// Set starting camera position
	position.x = 0;
	position.y = 5;
	position.z = -100;
	// Set starting camera view direction (heading)
	heading.x = 0;  // {0,0,1} for cubic environment mapping to work correctly
  heading.y = 0;
  heading.z = 1;
  Position_Init (&position, &heading, RUN_SPEED);

/*____________________________________________________________________
|
| Init 3D graphics
|___________________________________________________________________*/

  // Set projection matrix
	float fov = 60; // degrees field of view
	float near_plane = 0.1f;
	float far_plane = 1000;
  gx3d_SetProjectionMatrix (fov, near_plane, far_plane);

  gx3d_SetFillMode (gx3d_FILL_MODE_GOURAUD_SHADED);

  // Clear the 3D viewport to all black
  color.r = 0;
  color.g = 0;
  color.b = 0;
  color.a = 0;

/*____________________________________________________________________
|
| Load 3D models
|___________________________________________________________________*/

	// Load a 3D model																								
  gx3d_ReadLWO2File ("Objects\\ptree6.lwo", &obj_tree, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_tree = gx3d_InitTexture_File("Objects\\Images\\ptree_d512.bmp", "Objects\\Images\\ptree_d512_fa.bmp", 0);
	// Load the same model but make sure mipmapping of the texture is turned off
  gx3d_ReadLWO2File ("Objects\\tree2.lwo", &obj_tree2, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_GENERATE_MIPMAPS);

  gx3d_ReadLWO2File ("Objects\\skydome.lwo", &obj_skydome, gx3d_VERTEXFORMAT_DEFAULT, 0);
  gx3d_ReadLWO2File ("Objects\\clouddome.lwo", &obj_clouddome, gx3d_VERTEXFORMAT_DEFAULT, 0);

  gx3d_ReadLWO2File ("Objects\\ghost.lwo", &obj_ghost, gx3d_VERTEXFORMAT_DEFAULT, 0);
  
  gx3dObject* obj_ground;
  gx3d_ReadLWO2File("Objects\\ground.lwo", &obj_ground, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_ground = gx3d_InitTexture_File("Objects\\Images\\sand_d512.bmp", 0, 0);

  gx3dObject* obj_pyramid;
  gx3d_ReadLWO2File("Objects\\pyramid.lwo", &obj_pyramid, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_pyramid = gx3d_InitTexture_File("Objects\\Images\\brick.bmp", 0, 0);

  gx3dObject* obj_flower;
  gx3d_ReadLWO2File("Objects\\flower2.lwo", &obj_flower, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_flower = gx3d_InitTexture_File("Objects\\Images\\flower.bmp", "Objects\\Images\\flower_fa.bmp", 0);

  gx3dObject* obj_gastly;
  gx3d_ReadLWO2File("Objects\\gastly.lwo", &obj_gastly, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_gastly = gx3d_InitTexture_File("Objects\\Images\\gastly.bmp", "Objects\\Images\\gastly_fa.bmp", 0);
  gx3dTexture tex_ded_gastly = gx3d_InitTexture_File("Objects\\Images\\gastly_ded.bmp", "Objects\\Images\\gastly_ded_fa.bmp", 0);

  gx3dObject* obj_wasted;
  gx3d_ReadLWO2File("Objects\\wasted.lwo", &obj_wasted, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
  gx3dTexture tex_wasted = gx3d_InitTexture_File("Objects\\Images\\wasted.bmp", "Objects\\Images\\wasted_fa.bmp", 0);

  const int NUM_GASTLIES = 20;
  gx3dVector gastlyPosition[NUM_GASTLIES];
  float gastlySpeed[NUM_GASTLIES];
  boolean gastlyDraw[NUM_GASTLIES];
  gx3dSphere gastlySphere[NUM_GASTLIES];
  boolean gastlyOnScreen[NUM_GASTLIES];

  for (int i = 0; i < NUM_GASTLIES; i++) {
	  if(gastlyOnScreen[i])
		  gastlyPosition[i].x = (rand() % 100) - 50;
		  gastlyPosition[i].y = 0;
		  gastlyPosition[i].z = (rand() % 100) - 50;
		  gastlySpeed[i] = (i+1) * 0.075;
		  gastlyDraw[i] = true;
  }

  int num_Gastlies_touched = 0;
  int gastlyHits[NUM_GASTLIES] = { 0 };

  const int MAX_WASTED = 20;
  gx3dVector wastedPosition[MAX_WASTED];
  int wastedTimer[MAX_WASTED];
  int wastedIndex = 0;


  gx3d_GetScaleMatrix(&m, 5, 1, 5);
  gx3d_TransformObject(obj_flower, &m);

  //gastlyPosition[0].x = 0;
  //gastlyPosition[0].y = 5;
  //gastlyPosition[0].z = -20;
  //gastlySpeed[0] = 0.1f;
  //gastlyDraw[0] = true;

  //gastlyPosition[1].x = 0;
  //gastlyPosition[1].y = 0;
  //gastlyPosition[1].z = -10;
  //gastlySpeed[1] = 0.3f;
  //gastlyDraw[1] = true;

/*____________________________________________________________________
|
| create lights
|___________________________________________________________________*/

	// This code needs to be outside the main game loop.  That was the problem in class.

	gx3dLight dir_light;
	gx3dLightData light_data;
	light_data.light_type = gx3d_LIGHT_TYPE_DIRECTION;
	light_data.direction.diffuse_color.r = 1;
	light_data.direction.diffuse_color.g = 1;
	light_data.direction.diffuse_color.b = 1;
	light_data.direction.diffuse_color.a = 0;
	light_data.direction.specular_color.r = 1;
	light_data.direction.specular_color.g = 1;
	light_data.direction.specular_color.b = 1;
	light_data.direction.specular_color.a = 0;
	light_data.direction.ambient_color.r = 0;
	light_data.direction.ambient_color.g = 0;
	light_data.direction.ambient_color.b = 0;
	light_data.direction.ambient_color.a = 0;
	light_data.direction.dst.x = -1;
	light_data.direction.dst.y = -1;
	light_data.direction.dst.z = 0;

	dir_light = gx3d_InitLight (&light_data);


	gx3dLight point_light1;
	light_data.light_type =	gx3d_LIGHT_TYPE_POINT;
	light_data.point.diffuse_color.r = 1;  // red light
	light_data.point.diffuse_color.g = 1;
	light_data.point.diffuse_color.b = 1;
	light_data.point.diffuse_color.a = 0;
	light_data.point.specular_color.r = 1;
	light_data.point.specular_color.g = 1;
	light_data.point.specular_color.b = 1;
	light_data.point.specular_color.a = 0;
	light_data.point.ambient_color.r = 0;  // ambient turned offf
	light_data.point.ambient_color.g = 0;
	light_data.point.ambient_color.b = 0;
	light_data.point.ambient_color.a = 0;
	light_data.point.src.x = 10;
	light_data.point.src.y = 20;
	light_data.point.src.z = 0;
	light_data.point.range = 200;
	light_data.point.constant_attenuation = 0;
	light_data.point.linear_attenuation = 0.1;
	light_data.point.quadratic_attenuation = 0;

	point_light1 = gx3d_InitLight (&light_data);


	gx3dVector light_position = { 10, 20, 0 }, xlight_position;
	float angle = 0;

/*____________________________________________________________________
|
| Flush input queue
|___________________________________________________________________*/
    
	int move_x, move_y;	// mouse movement counters

	// Flush input queue
  evFlushEvents ();
	// Zero mouse movement counters
  msGetMouseMovement (&move_x, &move_y);  // call this here so the next call will get movement that has occurred since it was called here                                    
	// Hide mouse cursor
	msHideMouse ();
	
/*____________________________________________________________________
|
| Main game loop
|___________________________________________________________________*/

	boolean walking = false;



	snd_PlaySound(s_song, 1);
	snd_SetSoundVolume(s_song, 65);

	/*snd_SetSoundMode(s_ring, snd_3D_MODE_ORIGIN_RELATIVE, snd_3D_APPLY_NOW);
	snd_SetSoundPosition(s_ring, 0, 0, 0, snd_3D_APPLY_NOW);
	snd_SetSoundMinDistance(s_ring, 10, snd_3D_APPLY_NOW);
	snd_SetSoundMaxDistance(s_ring, 50, snd_3D_APPLY_NOW);
	snd_PlaySound(s_ring, 1);*/

	
	// Variables
  unsigned elapsed_time, last_time, new_time;
	bool force_update;
	unsigned cmd_move;

	// Init loop variables
	cmd_move = 0;
	last_time = 0;
	force_update = false;

	int tree_x[25], tree_z[25];
	for (int i = 0; i < 25; i++) {
		tree_x[i] = (rand() % 100) - 50;
		tree_z[i] = (rand() % 100) - 50;
	}

	int flower_x[200], flower_z[200];
	for (int i = 0; i < 200; i++) {
		flower_x[i] = (rand() % 100) - 50;
		flower_z[i] = (rand() % 100) - 50;
	}

	int lightMode = 0; 

	bool fastMovement = false;

	// Game loop
	for (quit=FALSE; NOT quit; ) {

		angle += 0.5;
		if (angle >= 360)
			angle = 0;
		gx3d_GetRotateYMatrix (&m, angle);
		gx3d_MultiplyVectorMatrix (&light_position, &m, &(light_data.point.src));
    gx3d_UpdateLight (point_light1, &light_data);

/*____________________________________________________________________
|
| Update clock
|___________________________________________________________________*/

		// Get the current time (# milliseconds since the program started)
    new_time = timeGetTime ();
		// Compute the elapsed time (in milliseconds) since the last time through this loop
    if (last_time == 0) 
      elapsed_time = 0;
    else 
      elapsed_time = new_time - last_time;
    last_time = new_time;

/*____________________________________________________________________
|
| Process user input
|___________________________________________________________________*/

		// Any event ready?
		if (evGetEvent (&event)) {
			// key press?
			if (event.type == evTYPE_RAW_KEY_PRESS)	{
				// If ESC pressed, exit the program
				if (event.keycode == evKY_ESC)
					quit = TRUE;
				else if (event.keycode == 'w')
					cmd_move |= POSITION_MOVE_FORWARD;
				else if (event.keycode == 's')
					cmd_move |= POSITION_MOVE_BACK;
				else if (event.keycode == 'a')
					cmd_move |= POSITION_MOVE_LEFT;
				else if (event.keycode == 'd')
					cmd_move |= POSITION_MOVE_RIGHT;
				else if (event.keycode == evKY_F1) {
					lightMode++;
					if (lightMode == 3)
						lightMode = 0;
				}
				else if (event.keycode == evKY_SHIFT) {
					fastMovement = true;
				}
				if (cmd_move != 0) {
					walking = true;
					if (!snd_IsPlaying(s_steps))
						snd_PlaySound(s_steps, 1);
					    snd_SetSoundVolume(s_steps, 60);
				}
			}
			// key release?
			else if (event.type == evTYPE_RAW_KEY_RELEASE) {
				if (event.keycode == 'w')
					cmd_move &= ~(POSITION_MOVE_FORWARD);
				else if (event.keycode == 's')
					cmd_move &= ~(POSITION_MOVE_BACK);
				else if (event.keycode == 'a')
					cmd_move &= ~(POSITION_MOVE_LEFT);
				else if (event.keycode == 'd')
					cmd_move &= ~(POSITION_MOVE_RIGHT);
				else if (event.keycode == evKY_SHIFT)
					fastMovement = false;
				if (cmd_move == 0) {
					walking = false;
					snd_StopSound(s_steps);
				}
			}
			// left mouse press
			else if (event.type == evTYPE_MOUSE_LEFT_RELEASE) {
				// Play sound effect 
				snd_PlaySound(s_pew, 0);
				snd_SetSoundVolume(s_pew, 60);
				// Collision test
				gx3dRay viewVector;
				viewVector.origin = position;
				viewVector.direction = heading;
				for (int i = 0; i < NUM_GASTLIES; i++) {
					if (gastlyOnScreen[i]) {
						gxRelation rel = gx3d_Relation_Ray_Sphere(&viewVector, &gastlySphere[i]);
						if (rel == gxRELATION_INSIDE && gxRELATION_INTERSECT) {
							//Sound effect
							snd_PlaySound(s_crash, 0);
							snd_SetSoundVolume(s_crash, 60);
							gastlyHits[i]++;

							// Check if hit 3 times
							if (gastlyHits[i] == 3) {
								// Remove Gastly
								gastlyDraw[i] = false;

								// Create wasted image
								wastedPosition[wastedIndex] = gastlySphere[i].center;
								wastedTimer[wastedIndex] = 1000 + elapsed_time;
								wastedIndex = (wastedIndex + 1) % MAX_WASTED;

								num_Gastlies_touched++;
								if (num_Gastlies_touched > 11)
									num_Gastlies_touched = 11;
							}
						}
					}
				}
			}
		}
		// Check for camera movement (via mouse)
		msGetMouseMovement (&move_x, &move_y);

/*____________________________________________________________________
|
| Update camera view
|___________________________________________________________________*/

		if (fastMovement)
			Position_Set_Speed(RUN_SPEED * 5);
		else
			Position_Set_Speed(RUN_SPEED);


    bool position_changed, camera_changed;
	// Took off inverse y axis camera setting
    Position_Update (elapsed_time, cmd_move, move_y, move_x, force_update, 
                     &position_changed, &camera_changed, &position, &heading);
	snd_SetListenerPosition(position.x, position.y, position.z, snd_3D_APPLY_NOW);
	snd_SetListenerOrientation(heading.x, heading.y, heading.z, 0, 1, 0, snd_3D_APPLY_NOW);
/*____________________________________________________________________
|
| Draw 3D graphics
|___________________________________________________________________*/

		// Render the screen
		gx3d_ClearViewport (gx3d_CLEAR_SURFACE | gx3d_CLEAR_ZBUFFER, color, gx3d_MAX_ZBUFFER_VALUE, 0);
    // Start rendering in 3D
    if (gx3d_BeginRender ()) {   
	  // Set the default material
	  gx3d_SetMaterial(&material_default);

      // Set the default light
      gx3d_SetAmbientLight (color3d_dim);
	  gx3d_EnableLight (dir_light);
	  //gx3d_EnableLight (point_light1);

	  switch (lightMode) {
	  case 0:
		  gx3d_SetAmbientLight(color3d_white);
		  gx3d_DisableLight(dir_light);
		  gx3d_DisableLight(point_light1);
		  break;
	  case 1:
		  gx3d_SetAmbientLight(color3d_dim);
		  gx3d_DisableLight(point_light1);
		  gx3d_EnableLight(dir_light);
		  break;
	  case 2:
		  gx3d_SetAmbientLight(color3d_dim);
		  gx3d_DisableLight(dir_light);
		  gx3d_EnableLight(point_light1);
		  break;
	  }

	  // Draw ground
	  gx3d_GetTranslateMatrix(&m, 0, 0, 0);
	  gx3d_SetObjectMatrix(obj_ground, &m);
	  gx3d_SetTexture(0, tex_ground);
	  gx3d_DrawObject(obj_ground, 0);

	  // Enable alpha blending
	  gx3d_EnableAlphaBlending ();
	  gx3d_EnableAlphaTesting (128);

	  // Draw a pyramid
	  gx3d_GetRotateYMatrix(&m1, 45);
	  gx3d_GetTranslateMatrix(&m2, 0, 0, 0);
	  gx3d_MultiplyMatrix(&m1, &m2, &m);
	  gx3d_SetObjectMatrix(obj_pyramid, &m);
	  gx3d_SetTexture(0, tex_pyramid);
	  gx3d_DrawObject(obj_pyramid, 0);

			// Draw a smaller tree
			gxRelation relation;
			gxRelation relation2;

			gx3dSphere sphere;
			sphere = obj_flower->bound_sphere;
			sphere.center.x += 0;
			sphere.radius += 70;
			relation = gx3d_Relation_Sphere_Frustum(&sphere);

			gx3dBox box;
			box = obj_tree->bound_box;
			box.min.x += -75;
			box.max.x += 75;
			gx3d_GetIdentityMatrix(&m1);
			relation2 = gx3d_Relation_Box_Frustum(&box, &m1);

			if (relation != gxRELATION_OUTSIDE) {
				for (int i = 0; i < 200; i++) {
					gx3d_GetTranslateMatrix(&m, flower_x[i], 0, flower_z[i]);
					gx3d_SetObjectMatrix(obj_flower, &m);
					gx3d_SetTexture(0, tex_flower);
					gx3d_DrawObject(obj_flower, 0);
				}
				debug_WriteFile("drawing");
			}
			else {
				debug_WriteFile("not drawing");
			}

			if (relation2 != gxRELATION_OUTSIDE) {
				for (int i = 0; i < 25; i++) {
					gx3d_GetTranslateMatrix(&m, tree_x[i], 0, tree_z[i]);
					gx3d_SetObjectMatrix(obj_tree, &m);
					gx3d_SetTexture(0, tex_tree);
					gx3d_DrawObject(obj_tree, 0);
				}
				debug_WriteFile ("drawing");
			}
			else {
				debug_WriteFile("not drawing");
			}

			/*gx3d_SetAmbientLight(color3d_white);
			gx3d_DisableLight(dir_light);*/

			// Alternate position of gastly
			for (int i= 0; i < NUM_GASTLIES; i++) {
					gastlyPosition[i].x += gastlySpeed[i];
					if (gastlyPosition[i].x > 50)
						gastlySpeed[i] = -gastlySpeed[i];
					else if (gastlyPosition[i].x < -50)
						gastlySpeed[i] = -gastlySpeed[i];
			}

			/*static float targetX = -10;
			static float targetX_incr = 0.1f;
			targetX = targetX_incr;
			if (targetX > 10)
				targetX_incr = -0.1f;
			else if (targetX < -10)
				targetX_incr = 0.1f;*/

			gxRelation relation3;

			for (int i = 0; i < NUM_GASTLIES; i++) {
				if (gastlyDraw[i]) {
					gastlySphere[i] = obj_gastly->bound_sphere;
					gastlySphere[i].center.x *= 60;
					gastlySphere[i].center.y *= 60;
					gastlySphere[i].center.z *= 60;
					gastlySphere[i].center.x = gastlyPosition[i].x;
					gastlySphere[i].center.y = gastlyPosition[i].y;
					gastlySphere[i].center.z = gastlyPosition[i].z;
					gastlySphere[i].radius *= 40;
					relation3 = gx3d_Relation_Sphere_Frustum(&gastlySphere[i]);
				}
			}

			// Draw gastly
			static gx3dVector billboard_normal = { 0,0,1 };
			for (int i = 0; i < NUM_GASTLIES; i++) {
				gastlyOnScreen[i] = false;
				if (gastlyDraw[i]) {
					if (relation3 != gxRELATION_OUTSIDE) {
						gx3d_GetScaleMatrix(&m1, 6, 6, 6);
						gx3d_GetBillboardRotateYMatrix(&m2, &billboard_normal, &heading);
						gx3d_GetTranslateMatrix(&m3, gastlyPosition[i].x, gastlyPosition[i].y, gastlyPosition[i].z);
						gx3d_MultiplyMatrix(&m1, &m2, &m);
						gx3d_MultiplyMatrix(&m, &m3, &m);
						gx3d_SetObjectMatrix(obj_gastly, &m);
						gx3d_SetTexture(0, tex_gastly);
						gx3d_DrawObject(obj_gastly, 0);
						gastlyOnScreen[i] = true;
					}
				}
			}

			// Enable directional lighting
			gx3d_EnableLight(dir_light);
			gx3d_SetAmbientLight(color3d_dim);
		
			gx3d_DisableAlphaTesting ();

			// Draw skydome
			gx3d_SetAmbientLight (color3d_white);
			gx3d_DisableLight (dir_light);
			gx3d_GetScaleMatrix (&m1, 500, 500, 500);
			gx3d_SetObjectMatrix (obj_skydome, &m1);
			gx3d_DrawObject (obj_skydome, 0);

			// Draw clouds
			static float offset = 0;
			offset += 0.001;
			if (offset > 1.0)
				offset = 0;

			gx3d_GetTranslateTextureMatrix (&m1, offset, 0);
			gx3d_EnableTextureMatrix (0);
			gx3d_SetTextureMatrix (0, &m1);

			gx3d_GetScaleMatrix (&m1, 500, 500, 500);
			gx3d_SetObjectMatrix (obj_clouddome, &m1);
			gx3d_DrawObject (obj_clouddome, 0);

			// Wasted markers 

			const float WASTED_SCALE = 6;
			//Update wasted markers timers
			for (int i = 0; i < MAX_WASTED; i++) {
				if (wastedTimer[i] > 0)
					wastedTimer[i] -= elapsed_time;
			}

			// Draw wasted markers 
			gx3d_EnableAlphaBlending();
			gx3d_EnableAlphaTesting(128);
			for (int i = 0; i < MAX_WASTED; i++) {
				if (wastedTimer[i] > 0) {
					gx3d_GetTranslateTextureMatrix(&m1, 0, 0);
					gx3d_EnableTextureMatrix(0);
					gx3d_SetTextureMatrix(0, &m1);
					gx3d_GetScaleMatrix(&m1, WASTED_SCALE, WASTED_SCALE, WASTED_SCALE);
					gx3d_GetRotateYMatrix(&m2, 180);
					float y = wastedPosition[i].y + (1 - (wastedTimer[i] / 1000.0f)) * (2 * WASTED_SCALE);
					gx3d_GetTranslateMatrix(&m3, wastedPosition[i].x, y + 6, wastedPosition[i].z);
					gx3d_MultiplyMatrix(&m1, &m2, &m);
					gx3d_MultiplyMatrix(&m, &m3, &m);
					gx3d_SetObjectMatrix(obj_wasted, &m);
					gx3d_SetTexture(0, tex_wasted);
					gx3d_DrawObject(obj_wasted, 0);
				}
			}
			gx3d_DisableAlphaBlending();
			gx3d_DisableAlphaTesting();
/*____________________________________________________________________
|
| Draw 2D graphics on top of 3D
|___________________________________________________________________*/

			// Save current view matrix
			gx3dMatrix view_save;
			gx3d_GetViewMatrix(&view_save);

			// Set new view matrix
			gx3dVector tfrom = { 0,0,-1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
			gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
			gx3d_CameraSetViewMatrix();

			// Draw 2D icons on screen
			if (num_Gastlies_touched) {
				gx3d_DisableZBuffer();
				gx3d_EnableAlphaBlending();
				for (int i = 0; i < num_Gastlies_touched; i++) {
					gx3d_GetTranslateTextureMatrix(&m1, 0, 0);
					gx3d_EnableTextureMatrix(0);
					gx3d_SetTextureMatrix(0, &m1);
					gx3d_GetScaleMatrix(&m1, 0.05f, 0.05f, 0.05f);
					gx3d_GetRotateYMatrix(&m2, 180);
					gx3d_GetTranslateMatrix(&m3, -0.5 + (0.10 * i), 0.21, 0);
					gx3d_MultiplyMatrix(&m1, &m2, &m);
					gx3d_MultiplyMatrix(&m, &m3, &m);
					gx3d_SetObjectMatrix(obj_gastly, &m);
					gx3d_SetTexture(0, tex_ded_gastly);
					gx3d_DrawObject(obj_gastly, 0);
				}
				gx3d_DisableAlphaBlending();
				gx3d_EnableZBuffer();
			}

			gx3d_SetViewMatrix(&view_save);

			gx3d_EndRender();

			gxFlipVisualActivePages(FALSE);

			gx3d_DisableTextureMatrix(0);

	  }   
  }

/*____________________________________________________________________
|
| Free stuff and exit
|___________________________________________________________________*/

  //gx3d_FreeObject (obj_tree);  
  gx3d_FreeObject (obj_tree2);  
}

/*____________________________________________________________________
|
| Function: Init_Render_State
|
| Input: Called from Program_Run()
| Output: Initializes general 3D render state.
|___________________________________________________________________*/

static void Init_Render_State ()
{
  // Enable zbuffering
  gx3d_EnableZBuffer ();

  // Enable lighting
  gx3d_EnableLighting ();

  // Set the default alpha blend factor
  gx3d_SetAlphaBlendFactor (gx3d_ALPHABLENDFACTOR_SRCALPHA, gx3d_ALPHABLENDFACTOR_INVSRCALPHA);

  // Init texture addressing mode - wrap in both u and v dimensions
  gx3d_SetTextureAddressingMode (0, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
  gx3d_SetTextureAddressingMode (1, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
  // Texture stage 0 default blend operator and arguments
  gx3d_SetTextureColorOp (0, gx3d_TEXTURE_COLOROP_MODULATE, gx3d_TEXTURE_ARG_TEXTURE, gx3d_TEXTURE_ARG_CURRENT);
  gx3d_SetTextureAlphaOp (0, gx3d_TEXTURE_ALPHAOP_SELECTARG1, gx3d_TEXTURE_ARG_TEXTURE, 0);
  // Texture stage 1 is off by default
  gx3d_SetTextureColorOp (1, gx3d_TEXTURE_COLOROP_DISABLE, 0, 0);
  gx3d_SetTextureAlphaOp (1, gx3d_TEXTURE_ALPHAOP_DISABLE, 0, 0);
    
  // Set default texture coordinates
  gx3d_SetTextureCoordinates (0, gx3d_TEXCOORD_SET0);
  gx3d_SetTextureCoordinates (1, gx3d_TEXCOORD_SET1);

  // Enable trilinear texture filtering
  gx3d_SetTextureFiltering (0, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
  gx3d_SetTextureFiltering (1, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
}

/*____________________________________________________________________
|
| Function: Program_Free
|
| Input: Called from CMainFrame::OnClose()
| Output: Exits graphics mode.
|___________________________________________________________________*/

void Program_Free ()
{
  // Stop event processing 
  evStopEvents ();
  // Return to text mode 
  if (Pgm_system_font)
    gxFreeFont (Pgm_system_font);
  gxStopGraphics ();
}
