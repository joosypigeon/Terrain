#include <stdbool.h>

#include "raylib.h"

#include "raymath.h"

#include "rlgl.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include <stdlib.h>
#include <omp.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "camera.h"

#include "torus.h"
#include "terrain.h"

#define TORUS_MAJOR_SEGMENTS 128
#define TORUS_MINOR_SEGMENTS 64


int SCREEN_WIDTH;
int SCREEN_HEIGHT;
float HALF_SCREEN_WIDTH;
float HALF_SCREEN_HEIGHT;


int main(void)
{
    bool showWireframe = false;
size_t frameCounter = 0;
    size_t CELL_SIZE = 50;
    printf("Linked Raylib version: %s\n", RAYLIB_VERSION);
    const int glslVer = rlGetVersion();
    printf("GL version: %i\n", glslVer);

    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Fullscreen at Desktop Resolution");

    // Get the primary monitor's resolution before window creation
    int monitor = GetCurrentMonitor();
    SCREEN_HEIGHT = GetMonitorHeight(monitor);
    SCREEN_WIDTH = GetMonitorWidth(monitor);
    printf("Monitor %d: %d x %d\n", monitor, SCREEN_WIDTH, SCREEN_HEIGHT);
    SCREEN_WIDTH = (SCREEN_WIDTH/CELL_SIZE)*CELL_SIZE;
    SCREEN_HEIGHT = (SCREEN_HEIGHT/CELL_SIZE)*CELL_SIZE;
    printf("Monitor %d: %d x %d\n", monitor, SCREEN_WIDTH, SCREEN_HEIGHT);
    HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2.0f;
    HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2.0f;

    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, 0.0f};  // Positioned out along +Z axis
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };       // Looking at the quad at origin
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };           // Standard up direction
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load basic lighting shader
    Shader shader = LoadShader("src/lighting.vs","src/lighting.fs");
    // Get some required shader locations

    int locTime     = GetShaderLocation(shader, "time");
    int locAmp      = GetShaderLocation(shader, "waveAmplitude");
    int locFreq     = GetShaderLocation(shader, "waveFrequency");
    int locID       = GetShaderLocation(shader, "objectID");



    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    // NOTE: "matModel" location name is automatically assigned on shader loading, 
    // no need to get the location again if using that uniform name
    //shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    
    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_SCREEN_WIDTH, 200, -HALF_SCREEN_HEIGHT }, Vector3Zero(), YELLOW, shader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){ HALF_SCREEN_WIDTH, 200, HALF_SCREEN_HEIGHT }, Vector3Zero(), RED, shader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_SCREEN_WIDTH, 200, HALF_SCREEN_HEIGHT }, Vector3Zero(), GREEN, shader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){ HALF_SCREEN_WIDTH, 200, -HALF_SCREEN_HEIGHT }, Vector3Zero(), BLUE, shader);


    float R = SCREEN_WIDTH / (2.0f * PI);
    float r = SCREEN_HEIGHT / (2.0f * PI);
    SetTorusDimensions(R, r);
    Mesh torus_mesh = MyGenTorusMesh(TORUS_MAJOR_SEGMENTS, TORUS_MINOR_SEGMENTS);
    GenMeshTangents(&torus_mesh);
    Model torus_model = LoadModelFromMesh(torus_mesh);
    torus_model.materials[0].shader = shader;  // <== Required for lighting to take effect

    Mesh plane = MyGenFlatTorusMesh(TORUS_MAJOR_SEGMENTS, TORUS_MINOR_SEGMENTS);
    GenMeshTangents(&plane);
    Model terrain = LoadModelFromMesh(plane);
    terrain.materials[0].shader = shader;

    //int number_of_frame = 0;
    while (!WindowShouldClose())
    {
        float time = GetTime();
        frameCounter++;

        // Update camera
        UpdateCameraManual(&camera);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_Y)) { lights[0].enabled = !lights[0].enabled; }
        if (IsKeyPressed(KEY_R)) { lights[1].enabled = !lights[1].enabled; }
        if (IsKeyPressed(KEY_G)) { lights[2].enabled = !lights[2].enabled; }
        if (IsKeyPressed(KEY_B)) { lights[3].enabled = !lights[3].enabled; }
        
        // Update light values (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(shader, lights[i]);


        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                rlSetMatrixProjection(MatrixPerspective(
                    DEG2RAD * camera.fovy,
                    (float)SCREEN_WIDTH / SCREEN_HEIGHT,
                    10.0f,     // near clip
                    10000.0f   // far clip
                ));
                
                BeginShaderMode(shader);
                    int id = 1;
                    SetShaderValue(shader, locID, &id, SHADER_UNIFORM_INT);
                    DrawModel(torus_model, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                    // Object 0: slow low wave
                    float amp0 = 50.0f, freq0 = 10.0f; 
                    id = 0;
                    SetShaderValue(shader, locTime, &time, SHADER_UNIFORM_FLOAT);
                    SetShaderValue(shader, locAmp, &amp0, SHADER_UNIFORM_FLOAT);
                    SetShaderValue(shader, locFreq, &freq0, SHADER_UNIFORM_FLOAT);
                    SetShaderValue(shader, locID, &id, SHADER_UNIFORM_INT);
                    DrawModel(terrain, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                    

                    if(showWireframe)
                    {
                        id = 1;
                        SetShaderValue(shader, locID, &id, SHADER_UNIFORM_INT);
                        DrawModelWires(torus_model, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, DARKGRAY);
                        id = 0;
                        SetShaderValue(shader, locID, &id, SHADER_UNIFORM_INT);
                        DrawModelWires(terrain, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, DARKGRAY);
                    }
                EndShaderMode();

                // Draw spheres to show where the lights are
                for (int i = 0; i < MAX_LIGHTS; i++)
                {
                    if (lights[i].enabled) DrawSphereEx(lights[i].position, 10.0f, 8, 8, lights[i].color);
                    else DrawSphereWires(lights[i].position, 10.0f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
                }
            EndMode3D();


            DrawText("Boids with Predator Simulation", 20, 10, 20, DARKGRAY);
            DrawText("Current Resolution:", 20, 30, 20, DARKGRAY);
            DrawText(TextFormat("%d x %d", SCREEN_WIDTH, SCREEN_HEIGHT), 20, 50, 30, BLUE);

            DrawText(TextFormat("Frame Time: %0.2f ms", GetFrameTime() * 1000), 20, 110, 30, BLUE);
            DrawText(TextFormat("OpenMP threads: %d", omp_get_max_threads()), 20, 140, 30, BLUE);

            GuiCheckBox((Rectangle){ 20, 170, 28, 28 }, "Show Wires", &showWireframe);

            DrawFPS(SCREEN_WIDTH - 100, 10);



        EndDrawing();
    }

    CloseWindow();

    return 0;
}