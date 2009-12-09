typedef struct {
	
	F32	red;
	F32	green;
	F32	blue;
	
} Color3;

typedef struct {
	
	F32	X;
	F32	Y;
	F32	Z;
	
} Vector3;

typedef struct {
	
	F32	X;
	F32	Y;
	
} Vector2;

typedef struct {
	
	F32	red;
	F32	green;
	F32	blue;
	F32 alpha;
}  Color4;

typedef struct {

	Color3 waterColor;
	F32 waterFogDensityExponent;
	F32 underwaterFogModifier;
	Vector3 reflectionWaveletScale;
	F32 fresnelScale;
	F32 fresnelOffset;
	F32 refractScaleAbove;
	F32 refractScaleBelow;
	F32 blurMultiplier;
	Vector2 littleWaveDirection;
	Vector2 bigWaveDirection;
	LLUUID normalMapTexture;
	Color4 horizon;
	F32 hazeHorizon;
	Color4 blueDensity;
	F32 hazeDensity;
	F32 densityMultiplier;
	F32 distanceMultiplier;
	Color4 sunMoonColor;
	F32 sunMoonPosiiton;
	Color4 ambient;
	F32 eastAngle;
	F32 sunGlowFocus;
	F32 sunGlowSize;
	F32 sceneGamma;
	F32 starBrightness;
	Color4 cloudColor;
	Vector3 cloudXYDensity;
	F32 cloudCoverage;
	F32 cloudScale;
	Vector3 cloudDetailXYDensity;
	F32 cloudScrollX;
	F32 cloudScrollY;
	unsigned short maxAltitude;
	char cloudScrollXLock;
	char cloudScrollYLock;
	char drawClassicClouds;
	

} Meta7WindlightPacket;
