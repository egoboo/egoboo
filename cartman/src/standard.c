#include <time.h>

#define CLOCKRATE 14
#define SECONDRATE 1000

int     timclock = 0;
int     dunframe = 0;
int     secframe = 0;
Uint8   lastsecframe = 0;
volatile int    minsecframe = 10000;
int     worldclock = 0;
int     seed;
Uint16  entry;
char        charread;
FILE            *fileread;
char            nameread[80];
char        fpstext[256][8];

//--------------------------------------------------------------------------------------------
void frmtim( void )
{
    timclock++;
}

//--------------------------------------------------------------------------------------------
void sectim( void )
{
    if ( secframe < minsecframe )
    {
        minsecframe = secframe;
    }
    lastsecframe = secframe;
    secframe = 0;
}

//--------------------------------------------------------------------------------------------
void reset_minfps( void )
{
    minsecframe = 0;
    secframe = -9999;
    while ( minsecframe > -9999 )
    {
    }
    minsecframe = 10000;
    lastsecframe = 0;

    return;
}

//--------------------------------------------------------------------------------------------
void comment( void )
{
    fscanf( fileread, "%c", &charread );
    while ( charread != '\n' )
    {
        fscanf( fileread, "%c", &charread );
    }

    return;
}

//--------------------------------------------------------------------------------------------
void show_info( void )
{
    printf( "%s - %s\n", NAME, VERSION_STR );

    if ( timclock != 0 )
    {
        printf( "Seconds = %15f\n", ( timclock * CLOCKRATE ) / 1000.0 );
        printf( "Avg FPS = %15f\n", dunframe * 1000.0 / ( timclock * CLOCKRATE ) );
        if ( minsecframe != 10000 )
        {
            printf( "Min FPS = %8d.000000\n", minsecframe );
        }
    }
    printf( "\n\n" );

    return;
}

//--------------------------------------------------------------------------------------------
void fill_fpstext( void )
{
    int cnt;

    cnt = 0;
    while ( cnt < 256 )
    {
        fpstext[cnt][0] = 'F';
        fpstext[cnt][1] = 'P';
        fpstext[cnt][2] = 'S';
        fpstext[cnt][3] = ' ';
        if ( cnt >= 100 )
        {
            fpstext[cnt][4] = '0' + ( cnt / 100 );
        }
        else
        {
            fpstext[cnt][4] = ' ';
        }
        if ( cnt >= 10 )
        {
            fpstext[cnt][5] = '0' + (( cnt / 10 ) % 10 );
        }
        else
        {
            fpstext[cnt][5] = ' ';
        }
        fpstext[cnt][6] = '0' + ( cnt % 10 );
        fpstext[cnt][7] = 0;
        cnt++;
    }

    return;
}

//--------------------------------------------------------------------------------------------
void scan_pair( int *one, int *two )
{
    char temp;

    fscanf( fileread, "%d", one );
    fscanf( fileread, "%c", &temp );
    if ( temp == '-' )
    {
        fscanf( fileread, "%d", two );
        *two = ( *two ) - ( *one ) + 1;
        *two = abs( *two );
    }
    else
    {
        *two = 1;
    }

    return;
}

//--------------------------------------------------------------------------------------------
void limit( int low, int *value, int high )
{
    if ( *value < low )
    {
        *value = low;
        return;
    }
    if ( *value > high )
    {
        *value = high;
    }

    return;
}

//--------------------------------------------------------------------------------------------
