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

#define TORUS_MAJOR_SEGMENTS 256
#define TORUS_MINOR_SEGMENTS 128


int SCREEN_WIDTH;
int SCREEN_HEIGHT;
float HALF_SCREEN_WIDTH;
float HALF_SCREEN_HEIGHT;

Matrix MatrixTranslateVector(Vector3 v) {
    return MatrixTranslate(v.x, v.y, v.z);
}

void PrintMatrix(Matrix m) {
    printf("[ %6.2f %6.2f %6.2f %6.2f ]\n", m.m0,  m.m4,  m.m8,  m.m12);
    printf("[ %6.2f %6.2f %6.2f %6.2f ]\n", m.m1,  m.m5,  m.m9,  m.m13);
    printf("[ %6.2f %6.2f %6.2f %6.2f ]\n", m.m2,  m.m6,  m.m10, m.m14);
    printf("[ %6.2f %6.2f %6.2f %6.2f ]\n", m.m3,  m.m7,  m.m11, m.m15);
}


static bool printed = false;

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
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(shader, "matNormal");
    printf("shader.locs[SHADER_LOC_MATRIX_MVP]: %d\n", shader.locs[SHADER_LOC_MATRIX_MVP]);
    printf("shader.locs[SHADER_LOC_MATRIX_MODEL]: %d\n", shader.locs[SHADER_LOC_MATRIX_MODEL]);
    printf("shader.locs[SHADER_LOC_MATRIX_NORMAL]: %d\n", shader.locs[SHADER_LOC_MATRIX_NORMAL]);
    
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

    Mesh terrain_mesh = MyGenFlatTorusMesh(TORUS_MAJOR_SEGMENTS, TORUS_MINOR_SEGMENTS);
    GenMeshTangents(&terrain_mesh);
    Model terrain = LoadModelFromMesh(terrain_mesh);
    terrain.materials[0].shader = shader;


    Vector3 translation1 = { 2.0f * R, 0.0f, 0.0f };  // Move model to this position
    Vector3 translation2 = { 0.0, 1000.0f, 0.0f };

    //int number_of_frame = 0;
    while (!WindowShouldClose())
    {
        //float time = GetTime();
        frameCounter++;

        // Update camera
        UpdateCameraManual(&camera);

        // Projection matrix (pass aspect ratio)
        float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();


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
                    // --- Draw torus_model ---
                    Matrix torusModelMatrix = MatrixTranslateVector(translation2); 
                    Matrix view = GetCameraMatrix(camera);
                    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 10.0f, 10000.0f);
                    Matrix torusMVP = MatrixMultiply(projection, MatrixMultiply(view, torusModelMatrix));
                    Matrix torusNormalMatrix = MatrixTranspose(MatrixInvert(torusModelMatrix));

                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MVP], torusMVP);
                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MODEL], torusModelMatrix);
                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_NORMAL], torusNormalMatrix);

                    DrawModel(torus_model, Vector3Zero(), 1.0f, WHITE);

                    // --- Draw terrain model ---
                    Matrix terrainModelMatrix = MatrixTranslateVector(translation1);  
                    Matrix terrainMVP = MatrixMultiply(projection, MatrixMultiply(view, terrainModelMatrix));
                    Matrix terrainNormalMatrix = MatrixTranspose(MatrixInvert(terrainModelMatrix));


                    // Set shader values for terrain
                    // Note: We use the same shader for both models, so we can reuse the locations
                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MVP], terrainMVP);
                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MODEL], terrainModelMatrix);
                    SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_NORMAL], terrainNormalMatrix);

                    DrawModel(terrain, Vector3Zero(), 1.0f, WHITE);

                    if(!printed)
                    {
                        printf("torusMVP:\n");
                        PrintMatrix(torusMVP);
                        printf("torusModelMatrix:\n");
                        PrintMatrix(torusModelMatrix);
                        printf("torusNormalMatrix:\n");
                        PrintMatrix(torusNormalMatrix);
                        printf("terrainMVP:\n");
                        PrintMatrix(terrainMVP);
                        printf("terrainModelMatrix:\n");
                        PrintMatrix(terrainModelMatrix);
                        printf("terrainNormalMatrix:\n");
                        PrintMatrix(terrainNormalMatrix);
                        printed = true;
                    }

                    if(showWireframe)
                    {
                        DrawModelWires(torus_model, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, DARKGRAY);
                        rlPushMatrix();
                            Matrix transform = MatrixTranslateVector(translation1);
                            rlMultMatrixf(MatrixToFloat(transform));
                            rlEnableWireMode();
                            DrawModel(terrain, (Vector3){0, 0, 0}, 1.0f, DARKGRAY);  // Draw at origin because transform is handled manually
                        rlDisableWireMode();
                        rlPopMatrix();
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