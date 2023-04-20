/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/geometry/JointTransform.h"
#include "idlib/math/Quat.h"

#include "Game_local.h"

#include "anim/Anim.h"

#if MD5_BINARY_ANIM > 0
#include "framework/FileSystem.h"
#endif

bool idAnimManager::forceExport = false;

/***********************************************************************

	idMD5Anim

***********************************************************************/

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::idMD5Anim() {
	ref_count	= 0;
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	totaldelta.Zero();
}

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::~idMD5Anim() {
	Free();
}

/*
====================
idMD5Anim::Free
====================
*/
void idMD5Anim::Free( void ) {
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	name		= "";

	totaldelta.Zero();

	jointInfo.Clear();
	bounds.Clear();
	componentFrames.Clear();
}

/*
====================
idMD5Anim::NumFrames
====================
*/
int	idMD5Anim::NumFrames( void ) const {
	return numFrames;
}

/*
====================
idMD5Anim::NumJoints
====================
*/
int	idMD5Anim::NumJoints( void ) const {
	return numJoints;
}

/*
====================
idMD5Anim::Length
====================
*/
int idMD5Anim::Length( void ) const {
	return animLength;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const idVec3 &idMD5Anim::TotalMovementDelta( void ) const {
	return totaldelta;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const char *idMD5Anim::Name( void ) const {
	return name;
}

/*
====================
idMD5Anim::Reload
====================
*/
bool idMD5Anim::Reload( void ) {
	idStr filename;

	filename = name;
	Free();

	return LoadAnim( filename );
}

/*
====================
idMD5Anim::Allocated
====================
*/
size_t idMD5Anim::Allocated( void ) const {
	size_t	size = bounds.Allocated() + jointInfo.Allocated() + componentFrames.Allocated() + name.Allocated();
	return size;
}

#if MD5_BINARY_ANIM > 1

static unsigned int SwapInt32(unsigned int x) {
	return static_cast<unsigned int>((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
}

static float SwapFloat(float x) {
	union { float f; unsigned int ui32; } swapper;
	swapper.f = x;
	swapper.ui32 = SwapInt32(swapper.ui32);
	return swapper.f;
}

#define SWAP_FLOAT(f) f = SwapFloat(f)
#define SWAP_WHOLE(i) i = SwapInt32(i)

#else

#define SWAP_FLOAT(f)
#define SWAP_WHOLE(i)

#endif

#if MD5_BINARY_ANIM > 0

static const byte B_ANIM_MD5_VERSION = 101;
static const unsigned int B_ANIM_MD5_MAGIC = ('B' << 24) | ('M' << 16) | ('D' << 8) | B_ANIM_MD5_VERSION;

/* ====================
idMD5Anim::LoadBinary
==================== */
bool idMD5Anim::LoadBinary(const char* filename, const char* baseFolder) {

	idFile* file = NULL;
	unsigned int number;

	if (baseFolder && baseFolder[0]) {
		file = idLib::fileSystem->OpenFileRead(va("%s/%s", baseFolder, filename));
	} else {
		file = idLib::fileSystem->OpenFileRead(filename);
	}

	if (file == NULL) {
		common->Printf("LoadBinary: fileopen failure for %s.\n", filename); // TODO Development only?
		return false;
	}

	file->ReadUnsignedInt(number); SWAP_WHOLE(number);

	if (number != B_ANIM_MD5_MAGIC) { // 42 4D 44 65
		common->Printf("LoadBinary: magic number failure for %s %i.\n", filename, number); // TODO Development only?
		idLib::fileSystem->CloseFile(file); return false;
	}

	Free(); name = filename;

	file->ReadUnsignedInt(number); // ID_TIME_T 1/2
	file->ReadUnsignedInt(number); // ID_TIME_T 2/2

	file->ReadInt(numFrames);  SWAP_WHOLE(numFrames);
	file->ReadInt(frameRate);  SWAP_WHOLE(frameRate);
	file->ReadInt(animLength); SWAP_WHOLE(animLength);
	file->ReadInt(numJoints);  SWAP_WHOLE(numJoints);
	file->ReadInt(numAnimatedComponents); SWAP_WHOLE(numAnimatedComponents);
//	common->Printf("LoadBinary: numFrames %i\nframeRate %i\nanimLength %i\nnumJoints %i\nnumAnimatedComponents %i\n", numFrames, frameRate, animLength, numJoints, numAnimatedComponents);

	file->ReadUnsignedInt(number); SWAP_WHOLE(number); bounds.SetGranularity(1); bounds.SetNum(number);
	for (int i = 0; i < number; i++) {
		idBounds& b = bounds[i];
		file->ReadFloat(b[0].x); SWAP_FLOAT(b[0].x);
		file->ReadFloat(b[0].y); SWAP_FLOAT(b[0].y);
		file->ReadFloat(b[0].z); SWAP_FLOAT(b[0].z);
		file->ReadFloat(b[1].x); SWAP_FLOAT(b[1].x);
		file->ReadFloat(b[1].y); SWAP_FLOAT(b[1].y);
		file->ReadFloat(b[1].z); SWAP_FLOAT(b[1].z);
	}

	file->ReadUnsignedInt(number); SWAP_WHOLE(number); jointInfo.SetGranularity(1); jointInfo.SetNum(number);
	for (int i = 0; i < number; i++) {
		jointAnimInfo_t& j = jointInfo[i];
		idStr jointName; file->ReadString(jointName);
		if (jointName.IsEmpty()) {
			j.nameIndex = -1;
		} else {
			j.nameIndex = animationLib.JointIndex(jointName.c_str());
		}
		file->ReadInt(j.parentNum); SWAP_WHOLE(j.parentNum);
		file->ReadInt(j.animBits); SWAP_WHOLE(j.animBits);
		file->ReadInt(j.firstComponent); SWAP_WHOLE(j.firstComponent);
	//	common->Printf("\t\"%s\" %i %i\n", jointName.c_str(), j.parentNum, j.animBits, j.firstComponent);
	}

	file->ReadUnsignedInt(number); SWAP_WHOLE(number); baseFrame.SetGranularity(1); baseFrame.SetNum(number);
	for (int i = 0; i < number; i++) {
		idJointQuat& j = baseFrame[i];
		file->ReadFloat(j.q.x); SWAP_FLOAT(j.q.x);
		file->ReadFloat(j.q.y); SWAP_FLOAT(j.q.y);
		file->ReadFloat(j.q.z); SWAP_FLOAT(j.q.z);
		file->ReadFloat(j.q.w); SWAP_FLOAT(j.q.w);
		file->ReadVec3(j.t); // No swap.
	//	j.w = 0.00f;
	//	common->Printf("\t(%f %f %f) (%f %f %f)\n", j.t.x, j.t.y, j.t.z, j.q.x, j.q.y, j.q.z);
	}

	file->ReadUnsignedInt(number); SWAP_WHOLE(number); componentFrames.SetGranularity(1); componentFrames.SetNum(number + 1); // One extra to be able to read one more float than is necessary.
	for (int i = 0; i < componentFrames.Num(); i++) {
		file->ReadFloat(componentFrames[i]); // No swap.
	}

	file->ReadVec3(totaldelta); // No swap.

	#if 0
	int animLengthTest = ((numFrames - 1) * 1000 + frameRate - 1) / frameRate;
	if (animLength != animLengthTest) {
		common->Printf("ANIMLENGTH: %i > %i\n", animLength, animLengthTest);
		animLength = animLengthTest;
	}
	idVec3 totaldeltatest;
	if (numAnimatedComponents) {
		float* componentPtr = &componentFrames[jointInfo[0].firstComponent];
		if (jointInfo[0].animBits & ANIM_TX) {
			for (int i = 0; i < numFrames; i++) {
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.x;
			}
			totaldeltatest.x = componentPtr[numAnimatedComponents * (numFrames - 1)];
			componentPtr++;
		} else {
			totaldeltatest.x = 0.0f;
		}
		if (jointInfo[0].animBits & ANIM_TY) {
			for (int i = 0; i < numFrames; i++) {
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.y;
			}
			totaldeltatest.y = componentPtr[numAnimatedComponents * (numFrames - 1)];
			componentPtr++;
		} else {
			totaldeltatest.y = 0.0f;
		}
		if (jointInfo[0].animBits & ANIM_TZ) {
			for (int i = 0; i < numFrames; i++) {
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.z;
			}
			totaldeltatest.z = componentPtr[numAnimatedComponents * (numFrames - 1)];
		//	componentPtr++;
		} else {
			totaldeltatest.z = 0.0f;
		}
	} else {
		totaldeltatest.Zero();
	}
	if (totaldelta != totaldeltatest) {
		common ->Printf("TOTALDELTA: %f,%f,%f > %f,%f,%f / %f,%f,%f > 0 ~ %s\n", totaldelta.x, totaldelta.y, totaldelta.z, totaldeltatest.x, totaldeltatest.y, totaldeltatest.z, baseFrame[0].t.x, baseFrame[0].t.y, baseFrame[0].t.z, filename);
		totaldelta = totaldeltatest;
	}
	#endif

	baseFrame[0].t.Zero();

	idLib::fileSystem->CloseFile(file);

	return true;

}

#endif

/*
====================
idMD5Anim::LoadAnim
====================
*/
bool idMD5Anim::LoadAnim( const char *filename ) {
	int		version;
	idLexer	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken	token;
	int		i, j;
	int		num;

	#if MD5_BINARY_ANIM > 0
	if (LoadBinary(filename, idLexer::GetBaseFolder())) return true;
	#endif

	if ( !parser.LoadFile( filename ) ) {
		return false;
	}

	Free();

	name = filename;

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	numJoints = parser.ParseInt();
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse frame rate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate < 0 ) {
		parser.Error( "Invalid frame rate: %d", frameRate );
	}

	// parse number of animated components
	parser.ExpectTokenString( "numAnimatedComponents" );
	numAnimatedComponents = parser.ParseInt();
	if ( ( numAnimatedComponents < 0 ) || ( numAnimatedComponents > numJoints * 6 ) ) {
		parser.Error( "Invalid number of animated components: %d", numAnimatedComponents );
	}

	// parse the hierarchy
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( numJoints );
	parser.ExpectTokenString( "hierarchy" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		parser.ReadToken( &token );
		jointInfo[ i ].nameIndex = animationLib.JointIndex( token );

		// parse parent num
		jointInfo[ i ].parentNum = parser.ParseInt();
		if ( jointInfo[ i ].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", jointInfo[ i ].parentNum );
		}

		if ( ( i != 0 ) && ( jointInfo[ i ].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		jointInfo[ i ].animBits = parser.ParseInt();
		if ( jointInfo[ i ].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", jointInfo[ i ].animBits );
		}

		// parse first component
		jointInfo[ i ].firstComponent = parser.ParseInt();
		if ( ( numAnimatedComponents > 0 ) && ( ( jointInfo[ i ].firstComponent < 0 ) || ( jointInfo[ i ].firstComponent >= numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", jointInfo[ i ].firstComponent );
		}
	}

	parser.ExpectTokenString( "}" );

	// parse bounds
	parser.ExpectTokenString( "bounds" );
	parser.ExpectTokenString( "{" );
	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, bounds[ i ][ 0 ].ToFloatPtr() );
		parser.Parse1DMatrix( 3, bounds[ i ][ 1 ].ToFloatPtr() );
	}
	parser.ExpectTokenString( "}" );

	// parse base frame
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( numJoints );
	parser.ExpectTokenString( "baseframe" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		idCQuat q;
		parser.Parse1DMatrix( 3, baseFrame[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, q.ToFloatPtr() );//baseFrame[ i ].q.ToFloatPtr() );
		baseFrame[ i ].q = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();
	}
	parser.ExpectTokenString( "}" );

	// parse frames
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames );

	float *componentPtr = componentFrames.Ptr();
	for( i = 0; i < numFrames; i++ ) {
		parser.ExpectTokenString( "frame" );
		num = parser.ParseInt();
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		parser.ExpectTokenString( "{" );

		for( j = 0; j < numAnimatedComponents; j++, componentPtr++ ) {
			*componentPtr = parser.ParseFloat();
		}

		parser.ExpectTokenString( "}" );
	}

	// get total move delta
	if ( !numAnimatedComponents ) {
		totaldelta.Zero();
	} else {
		componentPtr = &componentFrames[ jointInfo[ 0 ].firstComponent ];
		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.x;
			}
			totaldelta.x = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.x = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.y;
			}
			totaldelta.y = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.y = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.z;
			}
			totaldelta.z = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
		} else {
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[ 0 ].t.Zero();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	// done
	return true;
}

/*
====================
idMD5Anim::IncreaseRefs
====================
*/
void idMD5Anim::IncreaseRefs( void ) const {
	ref_count++;
}

/*
====================
idMD5Anim::DecreaseRefs
====================
*/
void idMD5Anim::DecreaseRefs( void ) const {
	ref_count--;
}

/*
====================
idMD5Anim::NumRefs
====================
*/
int idMD5Anim::NumRefs( void ) const {
	return ref_count;
}

/*
====================
idMD5Anim::GetFrameBlend
====================
*/
void idMD5Anim::GetFrameBlend( int framenum, frameBlend_t &frame ) const {
	frame.cycleCount	= 0;
	frame.backlerp		= 0.0f;
	frame.frontlerp		= 1.0f;

	// frame 1 is first frame
	framenum--;
	if ( framenum < 0 ) {
		framenum = 0;
	} else if ( framenum >= numFrames ) {
		framenum = numFrames - 1;
	}

	frame.frame1 = framenum;
	frame.frame2 = framenum;
}

/*
====================
idMD5Anim::ConvertTimeToFrame
====================
*/
void idMD5Anim::ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const {
	int frameTime;
	int frameNum;

	if ( numFrames <= 1 ) {
		frame.frame1		= 0;
		frame.frame2		= 0;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	if ( time <= 0 ) {
		frame.frame1		= 0;
		frame.frame2		= 1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	frameTime			= time * frameRate;
	frameNum			= frameTime / 1000;
	frame.cycleCount	= frameNum / ( numFrames - 1 );

	if ( ( cyclecount > 0 ) && ( frame.cycleCount >= cyclecount ) ) {
		frame.cycleCount	= cyclecount - 1;
		frame.frame1		= numFrames - 1;
		frame.frame2		= frame.frame1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		return;
	}

	frame.frame1 = frameNum % ( numFrames - 1 );
	frame.frame2 = frame.frame1 + 1;
	if ( frame.frame2 >= numFrames ) {
		frame.frame2 = 0;
	}

	frame.backlerp	= ( frameTime % 1000 ) * 0.001f;
	frame.frontlerp	= 1.0f - frame.backlerp;
}

/*
====================
idMD5Anim::GetOrigin
====================
*/
void idMD5Anim::GetOrigin( idVec3 &offset, int time, int cyclecount ) const {
	frameBlend_t frame;

	offset = baseFrame[ 0 ].t;
	if ( !( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) ) {
		// just use the baseframe
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
		offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
		offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
		offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
	}

	if ( frame.cycleCount ) {
		offset += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetOriginRotation
====================
*/
void idMD5Anim::GetOriginRotation( idQuat &rotation, int time, int cyclecount ) const {
	frameBlend_t	frame;
	int				animBits;

	animBits = jointInfo[ 0 ].animBits;
	if ( !( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) ) ) {
		// just use the baseframe
		rotation = baseFrame[ 0 ].q;
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const float	*jointframe1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float	*jointframe2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( animBits & ANIM_TX ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TY ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TZ ) {
		jointframe1++;
		jointframe2++;
	}

	idQuat q1;
	idQuat q2;

	switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
		case ANIM_QX:
			q1.x = jointframe1[0];
			q2.x = jointframe2[0];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY:
			q1.y = jointframe1[0];
			q2.y = jointframe2[0];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QZ:
			q1.z = jointframe1[0];
			q2.z = jointframe2[0];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.z = jointframe1[1];
			q2.x = jointframe2[0];
			q2.z = jointframe2[1];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY|ANIM_QZ:
			q1.y = jointframe1[0];
			q1.z = jointframe1[1];
			q2.y = jointframe2[0];
			q2.z = jointframe2[1];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q1.z = jointframe1[2];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q2.z = jointframe2[2];
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
	}

	rotation.Slerp( q1, q2, frame.backlerp );
}

/*
====================
idMD5Anim::GetBounds
====================
*/
void idMD5Anim::GetBounds( idBounds &bnds, int time, int cyclecount ) const {
	frameBlend_t	frame;
	idVec3			offset;

	ConvertTimeToFrame( time, cyclecount, frame );

	bnds = bounds[ frame.frame1 ];
	bnds.AddBounds( bounds[ frame.frame2 ] );

	// origin position
	offset = baseFrame[ 0 ].t;
	if ( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) {
		const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
		const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		}
	}

	bnds[ 0 ] -= offset;
	bnds[ 1 ] -= offset;
}

/*
====================
idMD5Anim::GetInterpolatedFrame
====================
*/
void idMD5Anim::GetInterpolatedFrame( frameBlend_t &frame, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i, numLerpJoints;
	const float				*frame1;
	const float				*frame2;
	const float				*jointframe1;
	const float				*jointframe2;
	const jointAnimInfo_t	*infoPtr;
	int						animBits;
	idJointQuat				*blendJoints;
	idJointQuat				*jointPtr;
	idJointQuat				*blendPtr;
	int						*lerpIndex;

	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if ( !numAnimatedComponents ) {
		// just use the base frame
		return;
	}

	blendJoints = (idJointQuat *)_alloca16( baseFrame.Num() * sizeof( blendPtr[ 0 ] ) );
	lerpIndex = (int *)_alloca16( baseFrame.Num() * sizeof( lerpIndex[ 0 ] ) );
	numLerpJoints = 0;

	frame1 = &componentFrames[ frame.frame1 * numAnimatedComponents ];
	frame2 = &componentFrames[ frame.frame2 * numAnimatedComponents ];

	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		jointPtr = &joints[j];
		blendPtr = &blendJoints[j];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits ) {

			lerpIndex[numLerpJoints++] = j;

			jointframe1 = frame1 + infoPtr->firstComponent;
			jointframe2 = frame2 + infoPtr->firstComponent;

			switch( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
				case 0:
					blendPtr->t = jointPtr->t;
					break;
				case ANIM_TX:
					jointPtr->t.x = jointframe1[0];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointPtr->t.y;
					blendPtr->t.z = jointPtr->t.z;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TY:
					jointPtr->t.y = jointframe1[0];
					blendPtr->t.y = jointframe2[0];
					blendPtr->t.x = jointPtr->t.x;
					blendPtr->t.z = jointPtr->t.z;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TZ:
					jointPtr->t.z = jointframe1[0];
					blendPtr->t.z = jointframe2[0];
					blendPtr->t.x = jointPtr->t.x;
					blendPtr->t.y = jointPtr->t.y;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TX|ANIM_TY:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.y = jointframe1[1];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointframe2[1];
					blendPtr->t.z = jointPtr->t.z;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TX|ANIM_TZ:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.z = jointframe1[1];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.z = jointframe2[1];
					blendPtr->t.y = jointPtr->t.y;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TY|ANIM_TZ:
					jointPtr->t.y = jointframe1[0];
					jointPtr->t.z = jointframe1[1];
					blendPtr->t.y = jointframe2[0];
					blendPtr->t.z = jointframe2[1];
					blendPtr->t.x = jointPtr->t.x;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TX|ANIM_TY|ANIM_TZ:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.y = jointframe1[1];
					jointPtr->t.z = jointframe1[2];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointframe2[1];
					blendPtr->t.z = jointframe2[2];
					jointframe1 += 3;
					jointframe2 += 3;
					break;
			}

			switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
				case 0:
					blendPtr->q = jointPtr->q;
					break;
				case ANIM_QX:
					jointPtr->q.x = jointframe1[0];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointPtr->q.y;
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QY:
					jointPtr->q.y = jointframe1[0];
					blendPtr->q.y = jointframe2[0];
					blendPtr->q.x = jointPtr->q.x;
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QZ:
					jointPtr->q.z = jointframe1[0];
					blendPtr->q.z = jointframe2[0];
					blendPtr->q.x = jointPtr->q.x;
					blendPtr->q.y = jointPtr->q.y;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QY:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.y = jointframe1[1];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointframe2[1];
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QZ:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.z = jointframe1[1];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.z = jointframe2[1];
					blendPtr->q.y = jointPtr->q.y;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QY|ANIM_QZ:
					jointPtr->q.y = jointframe1[0];
					jointPtr->q.z = jointframe1[1];
					blendPtr->q.y = jointframe2[0];
					blendPtr->q.z = jointframe2[1];
					blendPtr->q.x = jointPtr->q.x;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QY|ANIM_QZ:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.y = jointframe1[1];
					jointPtr->q.z = jointframe1[2];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointframe2[1];
					blendPtr->q.z = jointframe2[2];
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
			}
		}
	}

	SIMDProcessor->BlendJoints( joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints );

	if ( frame.cycleCount ) {
		joints[ 0 ].t += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetSingleFrame
====================
*/
void idMD5Anim::GetSingleFrame( int framenum, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i;
	const float				*frame;
	const float				*jointframe;
	int						animBits;
	idJointQuat				*jointPtr;
	const jointAnimInfo_t	*infoPtr;

	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if ( ( framenum == 0 ) || !numAnimatedComponents ) {
		// just use the base frame
		return;
	}

	frame = &componentFrames[ framenum * numAnimatedComponents ];

	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		jointPtr = &joints[j];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits ) {

			jointframe = frame + infoPtr->firstComponent;

			if ( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {

				if ( animBits & ANIM_TX ) {
					jointPtr->t.x = *jointframe++;
				}

				if ( animBits & ANIM_TY ) {
					jointPtr->t.y = *jointframe++;
				}

				if ( animBits & ANIM_TZ ) {
					jointPtr->t.z = *jointframe++;
				}
			}

			if ( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {

				if ( animBits & ANIM_QX ) {
					jointPtr->q.x = *jointframe++;
				}

				if ( animBits & ANIM_QY ) {
					jointPtr->q.y = *jointframe++;
				}

				if ( animBits & ANIM_QZ ) {
					jointPtr->q.z = *jointframe;
				}

				jointPtr->q.w = jointPtr->q.CalcW();
			}
		}
	}
}

/*
====================
idMD5Anim::CheckModelHierarchy
====================
*/
void idMD5Anim::CheckModelHierarchy( const idRenderModel *model ) const {
	int	i;
	int	jointNum;
	int	parent;

	if ( jointInfo.Num() != model->NumJoints() ) {
		gameLocal.Error( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
	}

	const idMD5Joint *modelJoints = model->GetJoints();
	for( i = 0; i < jointInfo.Num(); i++ ) {
		jointNum = jointInfo[ i ].nameIndex;
		if ( modelJoints[ i ].name != animationLib.JointName( jointNum ) ) {
			gameLocal.Error( "Model '%s''s joint names don't match anim '%s''s", model->Name(), name.c_str() );
		}
		if ( modelJoints[ i ].parent ) {
			parent = modelJoints[ i ].parent - modelJoints;
		} else {
			parent = -1;
		}
		if ( parent != jointInfo[ i ].parentNum ) {
			gameLocal.Error( "Model '%s' has different joint hierarchy than anim '%s'", model->Name(), name.c_str() );
		}
	}
}

/***********************************************************************

	idAnimManager

***********************************************************************/

/*
====================
idAnimManager::idAnimManager
====================
*/
idAnimManager::idAnimManager() {
}

/*
====================
idAnimManager::~idAnimManager
====================
*/
idAnimManager::~idAnimManager() {
	Shutdown();
}

/*
====================
idAnimManager::Shutdown
====================
*/
void idAnimManager::Shutdown( void ) {
	animations.DeleteContents();
	jointnames.Clear();
	jointnamesHash.Free();
}

/*
====================
idAnimManager::GetAnim
====================
*/
idMD5Anim *idAnimManager::GetAnim( const char *name ) {
	idMD5Anim **animptrptr;
	idMD5Anim *anim;

	// see if it has been asked for before
	animptrptr = NULL;
	if ( animations.Get( name, &animptrptr ) ) {
		anim = *animptrptr;
	} else {
		idStr extension;
		idStr filename = name;

		filename.ExtractFileExtension( extension );
		if ( extension != MD5_ANIM_EXT ) {
			return NULL;
		}

		anim = new idMD5Anim();
		if ( !anim->LoadAnim( filename ) ) {
			gameLocal.Warning( "Couldn't load anim: '%s'", filename.c_str() );
			delete anim;
			anim = NULL;
		}
		animations.Set( filename, anim );
	}

	return anim;
}

/*
================
idAnimManager::ReloadAnims
================
*/
void idAnimManager::ReloadAnims( void ) {
	int			i;
	idMD5Anim	**animptr;

	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			( *animptr )->Reload();
		}
	}
}

/*
================
idAnimManager::JointIndex
================
*/
int	idAnimManager::JointIndex( const char *name ) {
	int i, hash;

	hash = jointnamesHash.GenerateKey( name );
	for ( i = jointnamesHash.First( hash ); i != -1; i = jointnamesHash.Next( i ) ) {
		if ( jointnames[i].Cmp( name ) == 0 ) {
			return i;
		}
	}

	i = jointnames.Append( name );
	jointnamesHash.Add( hash, i );
	return i;
}

/*
================
idAnimManager::JointName
================
*/
const char *idAnimManager::JointName( int index ) const {
	return jointnames[ index ];
}

/*
================
idAnimManager::ListAnims
================
*/
void idAnimManager::ListAnims( void ) const {
	int			i;
	idMD5Anim	**animptr;
	idMD5Anim	*anim;
	size_t		size;
	size_t		s;
	size_t		namesize;
	int			num;

	num = 0;
	size = 0;
	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			anim = *animptr;
			s = anim->Size();
			gameLocal.Printf( "%8zd bytes : %2d refs : %s\n", s, anim->NumRefs(), anim->Name() );
			size += s;
			num++;
		}
	}

	namesize = jointnames.Size() + jointnamesHash.Size();
	for( i = 0; i < jointnames.Num(); i++ ) {
		namesize += jointnames[ i ].Size();
	}

	gameLocal.Printf( "\n%zd memory used in %d anims\n", size, num );
	gameLocal.Printf( "%zd memory used in %d joint names\n", namesize, jointnames.Num() );
}

/*
================
idAnimManager::FlushUnusedAnims
================
*/
void idAnimManager::FlushUnusedAnims( void ) {
	int						i;
	idMD5Anim				**animptr;
	idList<idMD5Anim *>		removeAnims;

	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			if ( ( *animptr )->NumRefs() <= 0 ) {
				removeAnims.Append( *animptr );
			}
		}
	}

	for( i = 0; i < removeAnims.Num(); i++ ) {
		animations.Remove( removeAnims[ i ]->Name() );
		delete removeAnims[ i ];
	}
}
