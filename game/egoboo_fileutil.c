#include "egoboo_fileutil.h"

#include "log.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

const char *parse_filename  = NULL;

int   pairbase, pairrand;
float pairfrom, pairto;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IDSZ fget_idsz( FILE* fileread )
{
    // ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

    IDSZ idsz = IDSZ_NONE;
    char cTmp = fget_first_letter( fileread );
    if ( cTmp == '[' )
    {
        idsz = 0;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 15;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 10;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp << 5;
        cTmp = ( fgetc( fileread ) - 'A' ) & 0x1F;  idsz |= cTmp;
    }

    return idsz;
}

//--------------------------------------------------------------------------------------------
int fget_int( FILE* fileread )
{
    int iTmp = 0;
    if ( feof( fileread ) ) return iTmp;

    fscanf( fileread, "%d", &iTmp );
    return iTmp;
}

//--------------------------------------------------------------------------------------------
bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
    /// @details BB@> copy a line of arbitrary length, in chunks of length
    ///      sizeof(linebuffer)

    char linebuffer[64];
    if (NULL == fileread || NULL == filewrite) return bfalse;
    if ( feof(fileread) || feof(filewrite) ) return bfalse;

    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);

    while ( strlen(linebuffer) == sizeof(linebuffer) )
    {
        fgets(linebuffer, sizeof(linebuffer), fileread);
        fputs(linebuffer, filewrite);
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void goto_colon( FILE* fileread )
{
    // ZZ> This function moves a file read pointer to the next colon
    //    char cTmp;
    Uint32 ch = fgetc( fileread );

    //    fscanf(fileread, "%c", &cTmp);
    while ( ch != ':' )
    {
        if ( ch == EOF )
        {
            // not enough colons in file!
            log_error( "There are not enough colons in file! (%s)\n", parse_filename );
        }

        ch = fgetc( fileread );
    }
}

//--------------------------------------------------------------------------------------------
bool_t goto_colon_yesno( FILE* fileread )
{
    // ZZ> This function moves a file read pointer to the next colon, or it returns
    //     bfalse if there are no more
    char cTmp;

    do
    {
        if ( fscanf( fileread, "%c", &cTmp ) == EOF )
        {
            return bfalse;
        }
    }
    while ( cTmp != ':' );

    return btrue;
}

//--------------------------------------------------------------------------------------------
char fget_first_letter( FILE* fileread )
{
    // ZZ> This function returns the next non-whitespace character
    char cTmp;
    fscanf( fileread, "%c", &cTmp );

    while ( isspace( cTmp ) )
    {
        fscanf( fileread, "%c", &cTmp );
    }

    return cTmp;
}

//--------------------------------------------------------------------------------------------
bool_t fget_name( FILE* fileread,  char *szName, size_t max_len )
{
    // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    //     it for underscores.  The szName argument is rewritten with the null terminated
    //     string

    int read;
	Uint32 cnt;
    char cTmp;
    STRING szTmp;

    if (NULL == szName) return bfalse;
    szName[0] = '\0';

    if (NULL == fileread || (0 != ferror(fileread)) || feof(fileread) ) return bfalse;

    // limit the max length of the string!
    // return value if the number of fields read, not amount read from file
    read = fscanf( fileread, "%256s", szTmp );

    szName[0] = '\0';
    if ( read > 0 )
    {
        for ( cnt = 0; cnt < max_len - 1; cnt++ )
        {
            cTmp = szTmp[cnt];
            if ( '\0' == cTmp ) break;
            if ( '_'  == cTmp ) cTmp = ' ';

            szName[cnt] = cTmp;
        }
        szName[cnt] = '\0';
    }

    return ferror(fileread);
}

//--------------------------------------------------------------------------------------------
void ftruthf( FILE* filewrite, const char* text, Uint8 truth )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     btrue bfalse statements

    fprintf( filewrite, "%s", text );
    if ( truth )
    {
        fprintf( filewrite, "TRUE\n" );
    }
    else
    {
        fprintf( filewrite, "FALSE\n" );
    }
}

//--------------------------------------------------------------------------------------------
void fdamagf( FILE* filewrite, const char* text, Uint8 damagetype )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf( filewrite, "%s", text );
    if ( damagetype == DAMAGE_SLASH )
        fprintf( filewrite, "SLASH\n" );
    if ( damagetype == DAMAGE_CRUSH )
        fprintf( filewrite, "CRUSH\n" );
    if ( damagetype == DAMAGE_POKE )
        fprintf( filewrite, "POKE\n" );
    if ( damagetype == DAMAGE_HOLY )
        fprintf( filewrite, "HOLY\n" );
    if ( damagetype == DAMAGE_EVIL )
        fprintf( filewrite, "EVIL\n" );
    if ( damagetype == DAMAGE_FIRE )
        fprintf( filewrite, "FIRE\n" );
    if ( damagetype == DAMAGE_ICE )
        fprintf( filewrite, "ICE\n" );
    if ( damagetype == DAMAGE_ZAP )
        fprintf( filewrite, "ZAP\n" );
    if ( damagetype == DAMAGENULL )
        fprintf( filewrite, "NONE\n" );
}

//--------------------------------------------------------------------------------------------
void factiof( FILE* filewrite, const char* text, Uint8 action )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf( filewrite, "%s", text );
    if ( action == ACTIONDA )
        fprintf( filewrite, "WALK\n" );
    if ( action == ACTIONUA )
        fprintf( filewrite, "UNARMED\n" );
    if ( action == ACTIONTA )
        fprintf( filewrite, "THRUST\n" );
    if ( action == ACTIONSA )
        fprintf( filewrite, "SLASH\n" );
    if ( action == ACTIONCA )
        fprintf( filewrite, "CHOP\n" );
    if ( action == ACTIONBA )
        fprintf( filewrite, "BASH\n" );
    if ( action == ACTIONLA )
        fprintf( filewrite, "LONGBOW\n" );
    if ( action == ACTIONXA )
        fprintf( filewrite, "XBOW\n" );
    if ( action == ACTIONFA )
        fprintf( filewrite, "FLING\n" );
    if ( action == ACTIONPA )
        fprintf( filewrite, "PARRY\n" );
    if ( action == ACTIONZA )
        fprintf( filewrite, "ZAP\n" );
}

//--------------------------------------------------------------------------------------------
void fgendef( FILE* filewrite, const char* text, Uint8 gender )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     MALE FEMALE OTHER statements

    fprintf( filewrite, "%s", text );
    if ( gender == GENMALE )
        fprintf( filewrite, "MALE\n" );
    if ( gender == GENFEMALE )
        fprintf( filewrite, "FEMALE\n" );
    if ( gender == GENOTHER )
        fprintf( filewrite, "OTHER\n" );
}

//--------------------------------------------------------------------------------------------
void fpairof( FILE* filewrite, const char* text, int base, int rand )
{
    // ZZ> This function mimics fprintf in spitting out
    //     damage/stat pairs
    undo_pair( base, rand );
    fprintf( filewrite, "%s", text );
    fprintf( filewrite, "%4.2f-%4.2f\n", pairfrom, pairto );
}

//--------------------------------------------------------------------------------------------
void funderf( FILE* filewrite, const char* text, const char* usename )
{
    // ZZ> This function mimics fprintf in spitting out
    //     a name with underscore spaces
    char cTmp;
    int cnt;

    fprintf( filewrite, "%s", text );
    cnt = 0;
    cTmp = usename[0];
    cnt++;

    while ( cTmp != 0 )
    {
        if ( cTmp == ' ' )
        {
            fprintf( filewrite, "_" );
        }
        else
        {
            fprintf( filewrite, "%c", cTmp );
        }

        cTmp = usename[cnt];
        cnt++;
    }

    fprintf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void read_pair( FILE* fileread )
{
    // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
    char cTmp;
    float  fBase, fRand;

    fscanf( fileread, "%f", &fBase );  // The first number
    pairbase = fBase * 256;
    cTmp = fget_first_letter( fileread );  // The hyphen
    if ( cTmp != '-' )
    {
        // Not in correct format, so fail
        pairrand = 1;
        return;
    }

    fscanf( fileread, "%f", &fRand );  // The second number
    pairrand = fRand * 256;
    pairrand = pairrand - pairbase;
    if ( pairrand < 1 )
        pairrand = 1;
}

//--------------------------------------------------------------------------------------------
void undo_pair( int base, int rand )
{
    // ZZ> This function generates a damage/stat pair ( eg. 3-6.5f )
    //     from the base and random values.  It set pairfrom and
    //     pairto
    pairfrom = base / 256.0f;
    pairto = rand / 256.0f;
    if ( pairfrom < 0.0f )
    {
        pairfrom = 0.0f;
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }
    if ( pairto < 0.0f )
    {
        pairto = 0.0f;
        log_warning( "We got a randomization error again! (Max is less than 0)\n" );
    }

    pairto += pairfrom;
}

//--------------------------------------------------------------------------------------------
void make_newloadname( const char *modname, const char *appendname,  char *newloadname )
{
    // ZZ> This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];
    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }

    tnc = 0;
    ctmp = appendname[tnc];

    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }

    newloadname[cnt] = 0;
}


