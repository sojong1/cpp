//======================================================================================================
// Motive API: Marker Tracking
//======================================================================================================

#include <conio.h>
#include <thread>
#include <mutex>

#include "MotiveAPI.h"

// for getting time
#include <windows.h> //windows.h 헤더 추가
#pragma comment(lib, "Winmm.lib") //winmm.lib 추가
#include <iostream>

// write .csv file
#include <fstream>
#include <string>

#include "motion_capture.h"


using namespace std::chrono_literals;
using namespace std;

// Local function prototypes
void CheckResult( eMotiveAPIResult result );
void ProcessFrame( int frame );

// Local constants
const float kRadToDeg = 0.0174532925f;

// write to a CSV file
std::ofstream ofile;


// Local class definitions
class Point4
{
public:
    Point4( float x, float y, float z, float w );

    float operator[]( int idx ) const { return mData[idx]; }
    const float* Data() const { return mData; }

private:
    float mData[4];
};

class TransformMatrix
{
public:
    TransformMatrix();

    TransformMatrix( float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44 );

    void SetTranslation( float x, float y, float z );
    void Invert();

    TransformMatrix operator*( const TransformMatrix& rhs );
    Point4          operator*( const Point4& v );

    static TransformMatrix RotateX( float rads );
    static TransformMatrix RotateY( float rads );
    static TransformMatrix RotateZ( float rads );

private:
    float mData[4][4];
};

class APIListener : public MotiveAPIListener
{
public:
    // Called when a camera connects to the system.
    void CameraConnected( int serialNumber ) override
    {
        CameraConnectedOrDisconnected( serialNumber, true );
    }

    // Called when a camera disconnects from the system.
    void CameraDisconnected( int serialNumber ) override
    {
        CameraConnectedOrDisconnected( serialNumber, false );
    }

    // Called by the API when a new frame is available
    void FrameAvailable() override
    {
        std::lock_guard<std::mutex> lock( mFrameAvailableMutex );

        mFrameAvailable = true;
        mFrameAvailableCond.notify_one();
    }

    // Blocks the main thread until a new frame is available from the API
    bool WaitForFrame( std::chrono::milliseconds timeout = 20ms )
    {
        bool frameAvailable = false;
        std::unique_lock<std::mutex> lock( mFrameAvailableMutex );

        frameAvailable = mFrameAvailableCond.wait_for( lock, timeout, [this] { return mFrameAvailable; } );

        mFrameAvailable = false;
        return frameAvailable;
    }

private:
    std::mutex mFrameAvailableMutex;
    std::condition_variable mFrameAvailableCond;
    bool mFrameAvailable = false;
    bool mVerbose = false;

    void CameraConnectedOrDisconnected( int serialNumber, bool connected )
    {
        if( mVerbose )
        {
            int cameraIndex = TT_CameraIndexFromSerial( serialNumber );
            if( cameraIndex >= 0 )
            {
                wchar_t cameraName[256];

                TT_CameraName( cameraIndex, cameraName, (int) sizeof( cameraName ) );
                if( connected )
                {
                    wprintf( L"APIListener - Camera Connected: %s\n", cameraName );
                }
                else
                {
                    wprintf( L"APIListener - Camera Disconnected: %s\n", cameraName );
                }
            }
        }
    }
};




// Main application
//int main( int argc, char* argv[] )
int logMotive()
{
    ofile.open("rawdata/motion_capture.csv");

    const wchar_t* calibrationFile = L"C:\\ProgramData\\OptiTrack\\Motive\\System Calibration.cal";
    const wchar_t* profileFile = L"C:\\ProgramData\\OptiTrack\\MotiveProfile.motive";

    printf( "=== Motive API Marker Tracking ===\n\n" );


    if( TT_Initialize() != kApiResult_Success )
    {
        printf( "Unable to license Motive API\n" );
        return 1;
    }

    // Attach listener for frame notifications
    APIListener listener;
    TT_AttachListener( &listener );

    // Load a camera calibration. For this example, we'll load the calibration that is automatically
    // saved by Motive when the system is calibrated.
    int cameraCount = 0;
    wprintf( L"Loading Calibration: \"%s\"\n\n", calibrationFile );
    CheckResult( TT_LoadCalibration( calibrationFile, &cameraCount ) );

    // Load a profile. For this example, we'll load the profile that is automatically
    // saved by Motive.
    wprintf( L"Loading Profile: \"%s\"\n\n", profileFile );
    CheckResult( TT_LoadProfile( profileFile ) );

    printf( "Initializing NaturalPoint Devices...\n\n" );

    // Wait until all the cameras from the calibration are available
    do
    {
        TT_Update();
        std::this_thread::sleep_for( 20ms );

    } while( TT_CameraCount() < cameraCount );

    // List all connected cameras
    printf( "Cameras:\n" );
    cameraCount = TT_CameraCount();
    for( int i = 0; i < cameraCount; i++ )
    {
        wchar_t name[256];

        TT_CameraName( i, name, 256 );
        wprintf( L"\t%s\n", name );
    }
    printf( "\n" );

    // List all defined rigid bodies.
    printf( "Rigid Bodies:\n" );
    int rigidBodyCount = TT_RigidBodyCount();
    for( int i = 0; i < rigidBodyCount; i++ )
    {
        wchar_t name[256];

        TT_RigidBodyName( i, name, 256 );
        wprintf( L"\t%s\n", name );
    }
    printf( "\n" );

    TT_FlushCameraQueues();

    int frameCounter = 0;
    bool running = true;
    
    //////////////////////////////////////////////////////////////////////////////
    // CSV header

    ofile << "frame#, time, ";

    for (int i = 1; i < 6; i++) {
        if (i < 5) {
            ofile << "Marker" << i << "_x, " << "Marker" << i << "_y, " << "Marker" << i << "_z, ";
        }
        else {
            ofile << "Marker" << i << "_x, " << "Marker" << i << "_y, " << "Marker" << i << "_z\n";
        }
    }

    //////////////////////////////////////////////////////////////////////////////




    // Process API data until the user hits a keyboard key.

    while( running )
    {
        // Blocks and waits for the next available frame.
        // TT_Update or TT_UpdateSingleFrame must be called in order to access the frame.
        // TT_Update will clear the frame queue, leaving only the most recent frame available.
        // TT_UpdateSingleFrame will only remove one frame from the queue with each call.
        bool frameAvailable = listener.WaitForFrame();



        while( TT_UpdateSingleFrame() == kApiResult_Success && frameAvailable )
        {
            ++frameCounter;

            // Update tracking information every 1 frames.
            if( ( frameCounter % 1) == 0 )
            {
                ProcessFrame(frameCounter);
            }
        }
    }

    // Save any changes
    CheckResult( TT_SaveProfile( profileFile ) );
    CheckResult( TT_SaveCalibration( calibrationFile ) );

    // Detach listener
    TT_DetachListener();

    // Shutdown API
    printf( "\n=== Shutting down NaturalPoint Motive API ===\n" );
    CheckResult( TT_Shutdown() );

    printf( "=== Complete ===\n" );
    while( !_kbhit() )
    {
        std::this_thread::sleep_for( 20ms );
    }

    return 0;
}

// Test method
void ProcessFrame( int frameCounter )
{
    float   yaw, pitch, roll;
    float   x, y, z;
    float   qx, qy, qz, qw;
    bool    tracked;


    int totalMarker = TT_FrameMarkerCount();
    printf("Frame #%d: %d Markers \n", frameCounter, totalMarker);
    ofile << frameCounter;


    ///////////////////// getTime ////////////////////////////
    unsigned long time = GetTickCount();
    // DWORD time = GetTickCount();
    // printf("GetTickCount : %l", time);
    cout << "\t timeGetTime : " << time << endl;
    ofile << "," << time;
    //////////////////////////////////////////////////////////


    for (int i = 0; i < totalMarker; i++) {
        double x = TT_FrameMarkerX(i);
        double y = TT_FrameMarkerY(i);
        double z = TT_FrameMarkerZ(i);

        printf("\t Marker: #%d:\t(%.2f,%.2f,%.2f)\n", i, x, y, z);
        ofile << "," << x << "," << y << "," << z;
    }
    ofile << "\n";
}

// CheckResult function will display errors and exit application.
void CheckResult( eMotiveAPIResult result )
{
    if( result != kApiResult_Success )
    {
        // Treat all errors as failure conditions.
        wprintf( L"Error: %s\n\n", TT_GetResultString( result ) );

        std::this_thread::sleep_for( 2000ms );
        exit( 1 );
    }
}

//
// Point4
//

Point4::Point4( float x, float y, float z, float w )
{
    mData[0] = x;
    mData[1] = y;
    mData[2] = z;
    mData[3] = w;
}

///////////////////////////////////////////////////////////
// 
// TransformMatrix 
// TransformMatrix
// TransformMatrix
//
///////////////////////////////////////////////////////////

TransformMatrix::TransformMatrix()
{
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            if( i == j )
            {
                mData[i][j] = 1.0f;
            }
            else
            {
                mData[i][j] = 0.0f;
            }
        }
    }
}

TransformMatrix::TransformMatrix( float m11, float m12, float m13, float m14,
    float m21, float m22, float m23, float m24,
    float m31, float m32, float m33, float m34,
    float m41, float m42, float m43, float m44 )
{
    mData[0][0] = m11;
    mData[0][1] = m12;
    mData[0][2] = m13;
    mData[0][3] = m14;
    mData[1][0] = m21;
    mData[1][1] = m22;
    mData[1][2] = m23;
    mData[1][3] = m24;
    mData[2][0] = m31;
    mData[2][1] = m32;
    mData[2][2] = m33;
    mData[2][3] = m34;
    mData[3][0] = m41;
    mData[3][1] = m42;
    mData[3][2] = m43;
    mData[3][3] = m44;
}

void TransformMatrix::SetTranslation( float x, float y, float z )
{
    mData[0][3] = x;
    mData[1][3] = y;
    mData[2][3] = z;
}

void TransformMatrix::Invert()
{
    // Exploit the fact that we are dealing with a rotation matrix + translation component.
    // http://stackoverflow.com/questions/2624422/efficient-4x4-matrix-inverse-affine-transform

    float   tmp;
    float   vals[3];

    // Transpose left-upper 3x3 (rotation) sub-matrix
    tmp = mData[0][1]; mData[0][1] = mData[1][0]; mData[1][0] = tmp;
    tmp = mData[0][2]; mData[0][2] = mData[2][0]; mData[2][0] = tmp;
    tmp = mData[1][2]; mData[1][2] = mData[2][1]; mData[2][1] = tmp;

    // Multiply translation component (last column) by negative inverse of upper-left 3x3.
    for( int i = 0; i < 3; ++i )
    {
        vals[i] = 0.0f;
        for( int j = 0; j < 3; ++j )
        {
            vals[i] += -mData[i][j] * mData[j][3];
        }
    }
    for( int i = 0; i < 3; ++i )
    {
        mData[i][3] = vals[i];
    }
}

TransformMatrix TransformMatrix::RotateX( float rads )
{
    return TransformMatrix( 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, (float) cos( rads ), (float) -sin( rads ), 0.0f,
        0.0f, (float) sin( rads ), (float) cos( rads ), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f );
}

TransformMatrix TransformMatrix::RotateY( float rads )
{
    return TransformMatrix( (float) cos( rads ), 0.0f, (float) sin( rads ), 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        (float) -sin( rads ), 0.0f, (float) cos( rads ), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f );
}

TransformMatrix TransformMatrix::RotateZ( float rads )
{
    return TransformMatrix( (float) cos( rads ), (float) -sin( rads ), 0.0f, 0.0f,
        (float) sin( rads ), (float) cos( rads ), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f );
}

TransformMatrix TransformMatrix::operator*( const TransformMatrix& rhs )
{
    TransformMatrix result;

    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            float rowCol = 0.0;
            for( int k = 0; k < 4; ++k )
            {
                rowCol += mData[i][k] * rhs.mData[k][j];
            }
            result.mData[i][j] = rowCol;
        }
    }
    return result;
}

Point4 TransformMatrix::operator*( const Point4& v )
{
    const float* pnt = v.Data();
    float   result[4];

    for( int i = 0; i < 4; ++i )
    {
        float rowCol = 0.0;
        for( int k = 0; k < 4; ++k )
        {
            rowCol += mData[i][k] * pnt[k];
        }
        result[i] = rowCol;
    }
    return Point4( result[0], result[1], result[2], result[3] );
}
