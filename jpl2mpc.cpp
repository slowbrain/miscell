#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

/* Code to convert an ephemeris gathered from JPL's _Horizons_ system into
the format used by MPC's DASO service.  Based largely on code I'd already
written to import _Horizons_ data to the .b32 format used in Guide.   */

int main( const int argc, const char **argv)
{
   FILE *ifile = (argc > 1 ? fopen( argv[1], "rb") : NULL);
   FILE *ofile = (argc > 2 ? fopen( argv[2], "wb") : stdout);
   char buff[200];
   unsigned n_written = 0;
   double jd0 = 0., step_size = 0., jd;
   bool state_vectors = false;

   if( argc > 1 && !ifile)
      printf( "\nCouldn't open the Horizons file '%s'\n", argv[1]);
   if( !ofile)
      printf( "\nCouldn't open the output file '%s'\n", argv[2]);

   if( argc < 2 || !ifile || !ofile)
      {
      printf( "\nJPL2MPC takes input ephemeri(de)s generated by HORIZONS,\n");
      printf( "and produces file(s) suitable for use in DASO.  The name of\n");
      printf( "the input ephemeris must be provided as a command-line argument.\n");
      printf( "For example:\n");
      printf( "\njpl2mpc gaia.txt\n\n");
      printf( "The JPL ephemeris must be in text form (can use the 'download/save'\n");
      printf( "option for this),  with vectors in units of AU.  Also,  the data\n");
      printf( "should be equatorial (not the default ecliptic!) coordinates.\n");
      printf( "You can also select position (xyz) output only,  if you wish,  but\n");
      printf( "you don't have to;  velocities and range/rate/light time will\n");
      printf( "simply be ignored if present.\n\n");
      printf( "   The bottom of 'jpl2mpc.cpp' shows how to submit a job via e-mail\n");
      printf( "to the Horizons server that will get you an ephemeris in the necessary\n");
      printf( "format.  You should be able to just cut and paste,  modifying the target\n");
      printf( "name and ephemeris range only.\n");
      exit( -1);
      }

   fseek( ifile, 0L, SEEK_SET);
   fprintf( ofile, "%13.5f %10.6f %4u\n", 0., 0., 0);
   while( fgets( buff, sizeof( buff), ifile))
      if( (jd = atof( buff)) > 2000000. && jd < 3000000. &&
               strlen( buff) > 54 && !memcmp( buff + 17, " = A.D.", 7)
               && !memcmp( buff + 42, ":00.0000 TDB", 12))
         {
         int xloc = 1, yloc = 24, zloc = 47;

         if( !fgets( buff, sizeof( buff), ifile))
            {
            printf( "Failed to get data from input file\n");
            return( -2);
            }
         if( !n_written)
            jd0 = jd;
         else if( n_written == 1)
            step_size = jd - jd0;
         if( buff[1] == 'X')     /* quantities are labelled */
            {
            xloc = 4;
            yloc = 30;
            zloc = 56;
            }
         fprintf( ofile, "%13.5f%16.10f%16.10f%16.10f", jd,
                  atof( buff + xloc), atof( buff + yloc), atof( buff + zloc));
         if( !state_vectors)
            printf( "\n");
         else
            {
            if( !fgets( buff, sizeof( buff), ifile))
               {
               printf( "Failed to get data from input file\n");
               return( -2);
               }
            fprintf( ofile, " %16.12f%16.12f%16.12f\n",
                  atof( buff + xloc), atof( buff + yloc), atof( buff + zloc));
            }
         n_written++;
         }
      else if( !memcmp( buff, "   VX    VY    VZ", 17))
         state_vectors = true;

   fprintf( ofile, "\n\nCreated from Horizons data by 'jpl2mpc', ver %s\n",
                                        __DATE__);
                     /* Seek back to start of input file & write header data: */
   fseek( ifile, 0L, SEEK_SET);
   while( fgets( buff, sizeof( buff), ifile) && memcmp( buff, "$$SOE", 5))
      fprintf( ofile, "%s", buff);

                     /* Seek back to start of file & write corrected hdr: */
   fseek( ofile, 0L, SEEK_SET);
   fprintf( ofile, "%13.5f %10.6f %4u\n", jd0, step_size, n_written);

   fclose( ifile);
// fprintf( ofile, "Ephemeris from JPL Horizons output\n");
// fprintf( ofile, "Created using 'jpl2mpc', version %s\n", __DATE__);
// fprintf( ofile, "Ephemeris converted %s", ctime( &t0));
// printf( "JD0: %f   Step size: %f   %ld steps\n",
//                             jd0,  step_size, (long)n_written);
   return( 0);
}

/* Following is an example e-mail request to the Horizons server for a
suitable text ephemeris for Gaia.  For other objects,  you would modify
the COMMAND and possibly CENTER lines (if you didn't want geocentric
vectors) as well as the START_TIME,  STOP_TIME, and STEP_SIZE. And,
of course,  the EMAIL_ADDR.

   Aside from that,  all is as it should be:  vectors are requested
with positions only,  with no light-time corrections,  in equatorial
J2000 coordinates,  in AU (not kilometers).

   After making those modifications,  you would send the result to
horizons@ssd.jpl.nasa.gov, subject line JOB.

!$$SOF (ssd)       JPL/Horizons Execution Control VARLIST
! Full directions are at
! ftp://ssd.jpl.nasa.gov/pub/ssd/horizons_batch_example.long

! EMAIL_ADDR sets e-mail address output is sent to. Enclose
! in quotes. Null assignment uses mailer return address.

 EMAIL_ADDR = 'pluto@projectpluto.com'
 COMMAND    = 'Gaia'

! MAKE_EPHEM toggles generation of ephemeris, if possible.
! Values: YES or NO

 MAKE_EPHEM = 'YES'

! TABLE_TYPE selects type of table to generate, if possible.
! Values: OBSERVER, ELEMENTS, VECTORS
! (or unique abbreviation of those values).

 TABLE_TYPE = 'VECTORS'
 CENTER     = '500@399'
 REF_PLANE  = 'FRAME'

! START_TIME specifies ephemeris start time
! (i.e. YYYY-MMM-DD {HH:MM} {UT/TT}) ... where braces "{}"
! denote optional inputs. See program user's guide for
! lists of the numerous ways to specify times. Time zone
! offsets can be set. For example, '1998-JAN-1 10:00 UT-8'
! would produce a table in Pacific Standard Time. 'UT+00:00'
! is the same as 'UT'. Offsets are not applied to TT
! (Terrestrial Time) tables. See TIME_ZONE variable also.

 START_TIME = '2014-OCT-14 00:00 TDB'

! STOP_TIME specifies ephemeris stop time
! (i.e. YYYY-MMM-DD {HH:MM}).

 STOP_TIME  = '2016-JAN-01'
 STEP_SIZE  = '1 day'
 QUANTITIES = '
 REF_SYSTEM = 'J2000'
 OUT_UNITS  = 'AU-D'

! VECT_TABLE = 1 means XYZ only,  no velocity, light-time,
! range, or range-rate.  Use VECT_TABLE = 2 to also get the
! velocity,  to produce state vector ephemerides resembling
! those from Find_Orb :
 VECT_TABLE = '1'

! VECT_CORR selects level of correction: NONE=geometric states
! (which we happen to want); 'LT' = astrometric states, 'LT+S'
! = same with stellar aberration included.
 VECT_CORR = 'NONE'

 CAL_FORMAT = 'CAL'

!$$EOF++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
