#include "egoboo_utility.h"

#include "char.h"
#include "particle.h"
#include "Mad.h"
#include "particle.h"
#include "Log.h"

#include "egoboo_strutil.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
DAMAGE fget_damage( FILE *fileread )
{
  char cTmp;
  DAMAGE retval = DAMAGE_NULL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'S': retval = DAMAGE_SLASH; break;
    case 'C': retval = DAMAGE_CRUSH; break;
    case 'P': retval = DAMAGE_POKE;  break;
    case 'H': retval = DAMAGE_HOLY;  break;
    case 'E': retval = DAMAGE_EVIL;  break;
    case 'F': retval = DAMAGE_FIRE;  break;
    case 'I': retval = DAMAGE_ICE;   break;
    case 'Z': retval = DAMAGE_ZAP;   break;
    case 'N': retval = DAMAGE_NULL;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
DAMAGE fget_next_damage( FILE *fileread )
{
  DAMAGE retval = DAMAGE_NULL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_damage( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
BLUD_LEVEL fget_blud( FILE *fileread )
{
  char cTmp;
  BLUD_LEVEL retval = BLUD_NORMAL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = BLUD_NORMAL; break;
    case 'F': retval = BLUD_NONE;   break;
    case 'U': retval = BLUD_ULTRA;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
BLUD_LEVEL fget_next_blud( FILE *fileread )
{
  BLUD_LEVEL retval = BLUD_NORMAL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_blud( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
RESPAWN_MODE fget_respawn( FILE *fileread )
{
  char cTmp;
  RESPAWN_MODE retval = RESPAWN_NORMAL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = RESPAWN_NORMAL;   break;
    case 'F': retval = RESPAWN_NONE;   break;
    case 'A': retval = RESPAWN_ANYTIME;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
RESPAWN_MODE fget_next_respawn( FILE *fileread )
{
  RESPAWN_MODE retval = RESPAWN_NORMAL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_respawn( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
DYNA_MODE fget_dynamode( FILE *fileread )
{
  char cTmp;
  DYNA_MODE retval = DYNA_OFF;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = DYNA_ON;   break;
    case 'F': retval = DYNA_OFF;   break;
    case 'L': retval = DYNA_LOCAL; break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
DYNA_MODE fget_next_dynamode( FILE *fileread )
{
  DYNA_MODE retval = DYNA_OFF;
  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_dynamode( fileread );
  }

  return retval;
};


//--------------------------------------------------------------------------------------------
char * undo_idsz( IDSZ idsz )
{
  // ZZ> This function takes an integer and makes an text IDSZ out of it.
  //     It will set valueidsz to "NONE" if the idsz is 0
  static char value_string[5] = {"NONE"};

  if ( idsz == IDSZ_NONE )
  {
    snprintf( value_string, sizeof( value_string ), "NONE" );
  }
  else
  {
    value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
    value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
    value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
    value_string[3] = (( idsz ) & 31 ) + 'A';
    value_string[4] = 0;
  }

  return value_string;
}

//--------------------------------------------------------------------------------------------
IDSZ fget_idsz( FILE* fileread )
{
  // ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one
  char sTemp[4], cTmp;
  IDSZ idsz = IDSZ_NONE;

  if ( feof( fileread ) ) return idsz;

  cTmp = fget_first_letter( fileread );
  if ( cTmp == '[' )
  {
    int cnt;
    for ( cnt = 0; cnt < 4; cnt++ ) sTemp[cnt] = fgetc( fileread );
    assert( ']' == fgetc( fileread ) );
    idsz = MAKE_IDSZ( sTemp );
  }

  return idsz;
}

//--------------------------------------------------------------------------------------------
IDSZ fget_next_idsz( FILE* fileread )
{
  // ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

  IDSZ idsz = IDSZ_NONE;

  if ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
  };

  return idsz;
}

//--------------------------------------------------------------------------------------------
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair )
{
  // ZZ> This function reads a damage/stat pair ( eg. 5-9 )

  char   cTmp;
  int    iBase, iRand;
  float  fBase, fRand;

  if ( feof( fileread ) ) return bfalse;

  fscanf( fileread, "%f", &fBase );   // The first number
  iBase = FLOAT_TO_FP8( fBase );

  cTmp = fget_first_letter( fileread );   // The hyphen
  if ( cTmp != '-' )
  {
    // second number is non-existant
    if ( NULL != ppair )
    {
      ppair->ibase = iBase;
      ppair->irand = 1;
    };
  }
  else
  {
    fscanf( fileread, "%f", &fRand );   // The second number
    iRand = FLOAT_TO_FP8( fRand );

    if ( NULL != ppair )
    {
      ppair->ibase = MIN( iBase, iRand );
      ppair->irand = ( MAX( iBase, iRand ) - ppair->ibase ) + 1;
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair )
{
  // ZZ> This function reads the next damage/stat pair ( eg. 5-9 )
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_pair_fp8( fileread, ppair );
  };

  return retval;

}

//--------------------------------------------------------------------------------------------
bool_t fget_pair( FILE* fileread, PAIR * ppair )
{
  // ZZ> This function reads a damage/stat pair ( eg. 5-9 )

  char   cTmp;
  int    iBase, iRand;
  float  fBase, fRand;

  if ( feof( fileread ) ) return bfalse;

  fscanf( fileread, "%f", &fBase );   // The first number
  iBase = fBase;

  cTmp = fget_first_letter( fileread );   // The hyphen
  if ( cTmp != '-' )
  {
    // second number is non-existant
    if ( NULL != ppair )
    {
      ppair->ibase = iBase;
      ppair->irand = 1;
    };
  }
  else
  {
    fscanf( fileread, "%f", &fRand );   // The second number
    iRand = fRand;

    if ( NULL != ppair )
    {
      ppair->ibase = ppair->ibase = MIN( iBase, iRand );
      ppair->irand = ppair->irand = ( MAX( iBase, iRand ) - ppair->ibase ) + 1;
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_pair( FILE* fileread, PAIR * ppair )
{
  // ZZ> This function reads the next damage/stat pair ( eg. 5-9 )
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_pair( fileread, ppair );
  };

  return retval;

}

//--------------------------------------------------------------------------------------------
bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange )
{
  // ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set prange->ffrom and
  //     prange->fto

  bool_t bfound = bfalse;
  float fFrom = 0.0f, fTo = 0.0f;

  if ( NULL != ppair )
  {
    fFrom = FP8_TO_FLOAT( ppair->ibase );
    fTo   = FP8_TO_FLOAT( ppair->ibase + ppair->irand - 1 );
    if ( fFrom < 0.0 )  fFrom = 0.0;
    if ( fTo   < 0.0 )  fTo   = 0.0;
    bfound = btrue;
  };

  if ( NULL != prange )
  {
    prange->ffrom = fFrom;
    prange->fto   = fTo;
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
bool_t undo_pair( PAIR * ppair, RANGE * prange )
{
  // ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set prange->ffrom and
  //     prange->fto

  bool_t bfound = bfalse;
  float fFrom = 0.0f, fTo = 0.0f;

  if ( NULL != ppair )
  {
    fFrom = ppair->ibase;
    fTo   = ppair->ibase + ppair->irand - 1;
    if ( fFrom < 0.0 )  fFrom = 0.0;
    if ( fTo   < 0.0 )  fTo   = 0.0;
    bfound = btrue;
  };

  if ( NULL != prange )
  {
    prange->ffrom = fFrom;
    prange->fto   = fTo;
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
void ftruthf( FILE* filewrite, char* text, Uint8 truth )
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     btrue bfalse statements

  fprintf( filewrite, text );
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
void fdamagf( FILE* filewrite, char* text, DAMAGE damagetype )
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
  fprintf( filewrite, text );

  switch ( damagetype )
  {
    case DAMAGE_SLASH: fprintf( filewrite, "SLASH\n" ); break;
    case DAMAGE_CRUSH: fprintf( filewrite, "CRUSH\n" ); break;
    case DAMAGE_POKE:  fprintf( filewrite, "POKE\n" );  break;
    case DAMAGE_HOLY:  fprintf( filewrite, "HOLY\n" );  break;
    case DAMAGE_EVIL:  fprintf( filewrite, "EVIL\n" );  break;
    case DAMAGE_FIRE:  fprintf( filewrite, "FIRE\n" );  break;
    case DAMAGE_ICE:   fprintf( filewrite, "ICE\n" );   break;
    case DAMAGE_ZAP:   fprintf( filewrite, "ZAP\n" );   break;
    default:           fprintf( filewrite, "NONE\n" );  break;
  };
}

//--------------------------------------------------------------------------------------------
void factiof( FILE* filewrite, char* text, ACTION action )
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

  fprintf( filewrite, text );

  if ( action >= ACTION_DA && action <= ACTION_DD )
    fprintf( filewrite, "WALK\n" );
  else if ( action >= ACTION_UA && action <= ACTION_UD )
    fprintf( filewrite, "UNARMED\n" );
  else if ( action >= ACTION_TA && action <= ACTION_TD )
    fprintf( filewrite, "THRUST\n" );
  else if ( action >= ACTION_SA && action <= ACTION_SD )
    fprintf( filewrite, "SLASH\n" );
  else if ( action >= ACTION_CA && action <= ACTION_CD )
    fprintf( filewrite, "CHOP\n" );
  else if ( action >= ACTION_BA && action <= ACTION_BD )
    fprintf( filewrite, "BASH\n" );
  else if ( action >= ACTION_LA && action <= ACTION_LD )
    fprintf( filewrite, "LONGBOW\n" );
  else if ( action >= ACTION_XA && action <= ACTION_XD )
    fprintf( filewrite, "XBOW\n" );
  else if ( action >= ACTION_FA && action <= ACTION_FD )
    fprintf( filewrite, "FLING\n" );
  else if ( action >= ACTION_PA && action <= ACTION_PD )
    fprintf( filewrite, "PARRY\n" );
  else if ( action >= ACTION_ZA && action <= ACTION_ZD )
    fprintf( filewrite, "ZAP\n" );
}

//--------------------------------------------------------------------------------------------
void fgendef( FILE* filewrite, char* text, GENDER gender )
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     MALE FEMALE OTHER statements

  fprintf( filewrite, text );

  switch ( gender )
  {
    case GEN_MALE:   fprintf( filewrite, "MALE" ); break;
    case GEN_FEMALE: fprintf( filewrite, "FEMALE" ); break;
    default:
    case GEN_OTHER:  fprintf( filewrite, "OTHER" ); break;
  };

  fprintf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void fpairof( FILE* filewrite, char* text, PAIR * ppair )
{
  // ZZ> This function mimics fprintf in spitting out
  //     damage/stat pairs
  RANGE rng;

  fprintf( filewrite, text );
  if ( undo_pair_fp8( ppair, &rng ) )
  {
    fprintf( filewrite, "%4.2f-%4.2f\n", rng.ffrom, rng.fto );
  }
  else
  {
    fprintf( filewrite, "0\n" );
  }

}

//--------------------------------------------------------------------------------------------
void funderf( FILE* filewrite, char* text, char* usename )
{
  // ZZ> This function mimics fprintf in spitting out
  //     a name with underscore spaces
  STRING tmpstr;

  str_convert_spaces( tmpstr, sizeof( tmpstr ), usename );
  fprintf( filewrite, "%s%s\n", text, tmpstr );
}

//--------------------------------------------------------------------------------------------
bool_t fget_message( FILE* fileread )
{
  // ZZ> This function loads a string into the message buffer, making sure it
  //     is null terminated.

  bool_t retval = bfalse;
  int cnt;
  char cTmp;
  STRING szTmp;

  if ( feof( fileread ) ) return retval;

  if ( GMsg.total >= MAXTOTALMESSAGE ) return retval;

  if ( GMsg.totalindex >= MESSAGEBUFFERSIZE )
  {
    GMsg.totalindex = MESSAGEBUFFERSIZE - 1;
  }

  // a zero return value from fscanf() means that no fields were filled
  if ( 0 != fscanf( fileread, "%s", szTmp ) )
  {
    GMsg.index[GMsg.total] = GMsg.totalindex;
    szTmp[255] = 0;
    cTmp = szTmp[0];
    cnt = 1;
    while ( cTmp != 0 && GMsg.totalindex < MESSAGEBUFFERSIZE - 1 )
    {
      if ( cTmp == '_' )  cTmp = ' ';
      GMsg.text[GMsg.totalindex] = cTmp;
      GMsg.totalindex++;
      cTmp = szTmp[cnt];
      cnt++;
    }
    GMsg.text[GMsg.totalindex] = 0;  GMsg.totalindex++;
    GMsg.total++;

    retval = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_message( FILE* fileread )
{
  // ZZ> This function loads a string into the message buffer, making sure it
  //     is null terminated.

  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_message( fileread );
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void fgoto_colon( FILE* fileread )
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
      log_error( "There are not enough colons in file! (%s)\n", globalname );
    }

    ch = fgetc( fileread );
  }
}

//--------------------------------------------------------------------------------------------
bool_t fgoto_colon_yesno( FILE* fileread )
{
  // ZZ> This function moves a file read pointer to the next colon, or it returns
  //     bfalse if there are no more
  char cTmp;
  bool_t bfound = bfalse;

  while ( !feof( fileread ) )
  {
    cTmp = fgetc( fileread );
    if ( ':' == cTmp )
    {
      bfound = btrue;
      break;
    };
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
char fget_first_letter( FILE* fileread )
{
  // ZZ> This function returns the next non-whitespace character
  char cTmp = '\0';
  bool_t bfound = bfalse;

  while ( !feof( fileread ) && !bfound )
  {
    cTmp = fgetc( fileread );
    bfound = isprint( cTmp ) && !isspace( cTmp );
  };

  return cTmp;
}

//--------------------------------------------------------------------------------------------
bool_t fget_name( FILE* fileread, char *szName, size_t lnName )
{
  // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
  //     it for underscores.  The szName argument is rewritten with the null terminated
  //     string

  bool_t retval = bfalse;
  STRING szTmp;

  if ( feof( fileread ) ) return retval;

  // a zero return value from fscanf() means that no fields were filled
  if ( fget_string( fileread, szTmp, sizeof( szTmp ) ) )
  {
    str_convert_underscores( szName, lnName, szTmp );
    retval = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName )
{
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_name( fileread, szName, lnName );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
int fget_int( FILE* fileread )
{
  int iTmp = 0;

  if ( feof( fileread ) ) return iTmp;

  fscanf( fileread, "%d", &iTmp );

  return iTmp;
};

//--------------------------------------------------------------------------------------------
int fget_next_int( FILE* fileread )
{
  int iTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    iTmp = fget_int( fileread );
  };

  return iTmp;
};

//--------------------------------------------------------------------------------------------
float fget_float( FILE* fileread )
{
  float fTmp = 0;

  if ( feof( fileread ) ) return fTmp;

  fscanf( fileread, "%f", &fTmp );

  return fTmp;
};

//--------------------------------------------------------------------------------------------
float fget_next_float( FILE* fileread )
{
  float fTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    fTmp = fget_float( fileread );
  };

  return fTmp;
};

//--------------------------------------------------------------------------------------------
Uint16 fget_fixed( FILE* fileread )
{
  float fTmp = 0;

  if ( feof( fileread ) ) return fTmp;

  fscanf( fileread, "%f", &fTmp );

  return FLOAT_TO_FP8( fTmp );
};

//--------------------------------------------------------------------------------------------
Uint16 fget_next_fixed( FILE* fileread )
{
  Uint16 iTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    iTmp = fget_fixed( fileread );
  };

  return iTmp;
};

//--------------------------------------------------------------------------------------------
bool_t fget_bool( FILE* fileread )
{
  bool_t bTmp = bfalse;
  char cTmp;

  if ( feof( fileread ) ) return bTmp;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': bTmp = btrue; break;
    case 'F': bTmp = bfalse; break;
  };

  return bTmp;
};

//--------------------------------------------------------------------------------------------
bool_t fget_next_bool( FILE* fileread )
{
  bool_t bTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    bTmp = fget_bool( fileread );
  };

  return bTmp;
};

//--------------------------------------------------------------------------------------------
GENDER fget_gender( FILE* fileread )
{
  char cTmp;
  GENDER retval = GEN_RANDOM;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'F': retval = GEN_FEMALE; break;
    case 'M': retval = GEN_MALE;   break;
    case 'O': retval = GEN_OTHER;  break;
    case 'R': retval = GEN_RANDOM; break;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
GENDER fget_next_gender( FILE* fileread )
{
  GENDER retval = GEN_RANDOM;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_gender( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
ACTION fget_action( FILE* fileread )
{
  char cTmp;
  ACTION retval = ACTION_DA;
  if ( feof( fileread ) ) return retval;


  cTmp = fget_first_letter( fileread );
  return what_action( cTmp );
}

//--------------------------------------------------------------------------------------------
ACTION fget_next_action( FILE* fileread )
{
  ACTION retval = ACTION_DA;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_action( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PRTTYPE fget_prttype( FILE * fileread )
{
  char cTmp;
  PRTTYPE retval = PRTTYPE_SOLID;

  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'L': retval = PRTTYPE_LIGHT; break;
    case 'S': retval = PRTTYPE_SOLID; break;
    case 'T': retval = PRTTYPE_ALPHA; break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PRTTYPE fget_next_prttype( FILE * fileread )
{
  PRTTYPE retval = PRTTYPE_SOLID;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_prttype( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine )
{
  // BB > this is a size-sensitive replacement for fscanf(fileread, "%s", &szLine)

  size_t len;
  char cTmp, *pout = szLine, *plast = szLine + lnLine;

  if ( feof( fileread ) ) return bfalse;
  if ( NULL == szLine || lnLine <= 1 ) return bfalse;

  // read past any initial spaces
  cTmp = '\0';
  while ( !feof( fileread ) )
  {
    cTmp = fgetc( fileread );
    if ( !isspace( cTmp ) ) break;
  }

  // stop at the next space or control character
  len = 0;
  while ( !feof( fileread ) && pout < plast )
  {
    if ( isspace( cTmp ) || !isprint( cTmp ) )
    {
      ungetc( cTmp, fileread );
      break;
    };
    *pout = cTmp;
    pout++;
    len++;
    cTmp = fgetc( fileread );
  };

  if ( pout < plast ) *pout = '\0';

  return len != 0;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine )
{
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_string( fileread, szLine, lnLine );
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
const char * inherit_fname(char * szObjPath, char * szObject, char *szFname )
{
  static STRING ret_fname;
  FILE * loc_pfile;
  STRING loc_fname, szTemp, inherit_line;
  bool_t found;

  STRING ifile;
  STRING itype;
  Uint32 icrc;

  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );
 
  // blank the static string
  ret_fname[0] = '\0';

  if(NULL == szObject)
  {
    // do not search
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);
    return ret_fname;
  }

  strcpy(loc_fname, szObjPath);
  str_append_slash(ret_fname, sizeof(ret_fname));
  strcat(loc_fname, szObject);
  str_append_slash( loc_fname, sizeof( loc_fname ) );
  strcat(loc_fname, "inherit.txt");
  loc_pfile = fs_fileOpen(PRI_NONE, "loc_fname()", loc_fname, "r");
  if(NULL == loc_pfile)
  {
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szObject);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);

    return ret_fname;
  };

   // try to match the given file
  found = bfalse;
  while( fget_next_string( loc_pfile, ifile, sizeof(STRING) ) )
  {
    if( 0 == strcmp(ifile, szFname) )
    {
      // "-"
      fget_string( loc_pfile, inherit_line, sizeof(STRING) );

      // CRC
      icrc = fget_int( loc_pfile );

      // "-"
      fget_string( loc_pfile, inherit_line, sizeof(STRING) );

      // type
      fget_string( loc_pfile, itype, sizeof(STRING) );

      found = btrue;

      break;
    }
  }

  if(!found)
  {
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szObject);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);
  }
  else
  {
    const char * name_ptr = NULL;

    sprintf(ret_fname, "%u*%s", icrc, itype);

    if( 0 == strcmp(".wav", itype) )
    {
      // search in the sounds directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_sounds" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_sounds" SLASH_STRING "%s", name_ptr);
      }

      fs_findClose(&fs_finfo);
    }
    else if ( 0 == strncmp( ifile, "icon" , 4 ) && 0 == strcmp(".bmp", itype) )
    {
      // search in the icons directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_icons" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_icons" SLASH_STRING "%s", name_ptr);
      }

      fs_findClose(&fs_finfo);
    }
    else if ( 0 == strncmp( ifile, "part" , 4 ) && 0 == strcmp(".txt", itype) )
    {
      // search in the particles directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_particles" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_particles" SLASH_STRING "%s", name_ptr);
      }

      fs_findClose(&fs_finfo);
    }
    else
    {
      STRING tmpfname;

      strcpy(tmpfname, "objects");
      str_append_slash( tmpfname, sizeof(tmpfname) );
      strcat(tmpfname, szObject );
      str_append_slash( tmpfname, sizeof(tmpfname) );

      name_ptr = fs_findFirstFile( &fs_finfo, tmpfname, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "%s%s", tmpfname, name_ptr);
      }

      fs_findClose(&fs_finfo);
    }
  };




  fs_fileClose(loc_pfile);

  return ret_fname;

}