extern "C" {
#include "dynamic_load.h"
}

#define TL_THUNDERLOOM_IMPLEMENTATION
#define TL_PUBLIC_FUNC_PREFIX DYNAMIC_FUNC_PREFIX
#include "thunderloom.h"

#define TL_PATTERN_EDITOR_IMPLEMENTATION
#include "pattern_editor.h"

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

// Disable some MSVC warnings for external headers.
#pragma warning( push )
#pragma warning( disable : 4251)
#pragma warning( disable : 4996 )
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"
#include "dbgprint.h"
#include "thunderloom.h"
#include "imtl.h"
#include "object.h"
#include "render.h"
#include "shadedata_new.h"
#pragma warning( pop ) 

#define _USE_MATH_DEFINES
#include <math.h>


//using namespace VUtils;

DYNAMIC_FUNC_PREFIX
void EvalDiffuseFunc (const VUtils::VRayContext *rc,
    tlWeaveParameters *weave_parameters, VUtils::ShadeCol *diffuse_color,
	VUtils::ShadeCol *opacity_color,
	tlYarnType* yarn_type, int* yarn_type_id,
	int *yarn_hit)
{
    if(weave_parameters->pattern == 0){ //Invalid pattern
        *diffuse_color = VUtils::ShadeCol(1.f,1.f,0.f);
        *opacity_color = VUtils::ShadeCol(1.f,1.f,1.f);
		*yarn_type = tl_default_yarn_type;
		*yarn_type_id = 0;
		*yarn_hit = 0;
        return;
    }
    tlIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(*rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;
    intersection_data.wi_z = 1.f;

    intersection_data.context = &sc;

    tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
        weave_parameters);
	*yarn_type_id = pattern_data.yarn_type;
	*yarn_type = weave_parameters->yarn_types[pattern_data.yarn_type];
	*yarn_hit = pattern_data.yarn_hit;
    tlColor d = 
        tl_eval_diffuse( intersection_data, pattern_data, weave_parameters);
	diffuse_color->set(d.r, d.g, d.b);
    tlColor o = 
        tl_eval_opacity( intersection_data, pattern_data, weave_parameters);
	opacity_color->set(o.r, o.g, o.b);
}

DYNAMIC_FUNC_PREFIX
void EvalSpecularFunc ( const VUtils::VRayContext *rc,
const VUtils::ShadeVec *direction, tlWeaveParameters *weave_parameters, VUtils::ShadeCol *reflection_color)
{
    if(weave_parameters->pattern == 0){ //Invalid pattern
		float s = 0.1f;
		reflection_color->set(s, s, s);
		return;
    }
    tlIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(*rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    //Convert the view and light directions to the correct coordinate system
    Point3 viewDir, lightDir;
    viewDir.x = -rc->rayparams.viewDir.x();
    viewDir.y = -rc->rayparams.viewDir.y();
    viewDir.z = -rc->rayparams.viewDir.z();
    viewDir = sc.VectorFrom(viewDir,REF_WORLD);
    viewDir = viewDir.Normalize();

    lightDir.x = direction->x();
    lightDir.y = direction->y();
    lightDir.z = direction->z();
    lightDir = sc.VectorFrom(lightDir,REF_WORLD);
    lightDir = lightDir.Normalize();

    // UVW derivatives
    Point3 dpdUVW[3];
    sc.DPdUVW(dpdUVW,1);

    Point3 n_vec = sc.Normal().Normalize();
    Point3 u_vec = dpdUVW[0].Normalize();
    Point3 v_vec = dpdUVW[1].Normalize();
    u_vec = (v_vec ^ n_vec).Normalize();
    v_vec = (n_vec ^ u_vec).Normalize();

    intersection_data.wi_x = DotProd(lightDir, u_vec);
    intersection_data.wi_y = DotProd(lightDir, v_vec);
    intersection_data.wi_z = DotProd(lightDir, n_vec);

    intersection_data.wo_x = DotProd(viewDir, u_vec);
    intersection_data.wo_y = DotProd(viewDir, v_vec);
    intersection_data.wo_z = DotProd(viewDir, n_vec);

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;

    intersection_data.context = &sc;

    tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
        weave_parameters);
    tlColor s = tl_eval_specular(intersection_data, pattern_data, weave_parameters);
    reflection_color->set(s.r, s.g, s.b);
}

DYNAMIC_FUNC_PREFIX
float EvalSampleFunc(const VUtils::VRayContext *rc,
	VUtils::ShadeVec *direction, tlWeaveParameters *weave_parameters, float *prob, float r) 
{
    if(weave_parameters->pattern == 0){ //Invalid pattern
		//TODO(Vidar):What to do here?
        return 0.f;
    }
    tlIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(*rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    //Convert the view and light directions to the correct coordinate system
    Point3 viewDir;
    viewDir.x = -rc->rayparams.viewDir.x();
    viewDir.y = -rc->rayparams.viewDir.y();
    viewDir.z = -rc->rayparams.viewDir.z();
    viewDir = sc.VectorFrom(viewDir,REF_WORLD);
    viewDir = viewDir.Normalize();

    // UVW derivatives
    Point3 dpdUVW[3];
    sc.DPdUVW(dpdUVW,1);

    Point3 n_vec = sc.Normal().Normalize();
    Point3 u_vec = dpdUVW[0].Normalize();
    Point3 v_vec = dpdUVW[1].Normalize();
    u_vec = (v_vec ^ n_vec).Normalize();
    v_vec = (n_vec ^ u_vec).Normalize();

    intersection_data.wo_x = DotProd(viewDir, u_vec);
    intersection_data.wo_y = DotProd(viewDir, v_vec);
    intersection_data.wo_z = DotProd(viewDir, n_vec);

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;

    intersection_data.context = &sc;

    tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
        weave_parameters);

	float fac = 1.f;
	tlVector vec = tl_sample(intersection_data, pattern_data, weave_parameters,r,&fac);

	//direction->set(vec.x, vec.y, vec.z);
	Point3 d = vec.x*u_vec + vec.y*v_vec + vec.z*n_vec;
	//Point3 d = -viewDir + n_vec * DotProd(n_vec, viewDir) * 2.f;
	//Point3 d = n_vec;
	//Point3 d = viewDir;
	//Point3 d = intersection_data.wo_x * u_vec + intersection_data.wo_y * v_vec + intersection_data.wo_z * n_vec;
	d = sc.VectorTo(d, REF_WORLD);
	direction->set(d.x, d.y, d.z);
	return fac;
}

class SCTexture: public ShadeContext {
	public:
		ShadeContext *origsc;
		TimeValue curtime;
		Point3 ltPos; // position of point in light space
		Point3 view;  // unit vector from light to point, in light space
		Point3 dp; 
		Point2 uv,duv;
		IPoint2 scrpos;
		float curve;
		int projType;

		BOOL 	  InMtlEditor() { return origsc->InMtlEditor(); }
		LightDesc* Light(int n) { return NULL; }
		TimeValue CurTime() { return curtime; }
		int NodeID() { return -1; }
		int FaceNumber() { return 0; }
		int ProjType() { return projType; }
		Point3 Normal() { return Point3(0,0,0); }
		Point3 GNormal() { return Point3(0,0,0); }
		Point3 ReflectVector(){ return Point3(0,0,0); }
		Point3 RefractVector(float ior){ return Point3(0,0,0); }
		Point3 CamPos() { return Point3(0,0,0); }
		Point3 V() { return view; }
		void SetView(Point3 v) { view = v; }
		Point3 P() { return ltPos; }	
		Point3 DP() { return dp; }
		Point3 PObj() { return ltPos; }
		Point3 DPObj(){ return Point3(0,0,0); } 
		Box3 ObjectBox() { return Box3(Point3(-1,-1,-1),Point3(1,1,1));}   	  	
		Point3 PObjRelBox() { return view; }
		Point3 DPObjRelBox() { return Point3(0,0,0); }
		void ScreenUV(Point2& UV, Point2 &Duv) { UV = uv; Duv = duv; }
		IPoint2 ScreenCoord() { return scrpos;} 
		Point3 UVW(int chan) { return Point3(uv.x, uv.y, 0.0f); }
		Point3 DUVW(int chan) { return Point3(duv.x, duv.y, 0.0f);  }
		void DPdUVW(Point3 dP[3], int chan) {}  // dont need bump vectors
		void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG=TRUE) {}   // returns Background color, bg transparency

#if MAX_VERSION_MAJOR >= 26 
		Matrix3 MatrixTo(RefFrame ito) { return origsc->MatrixTo(ito); }
		Matrix3 MatrixFrom(RefFrame ifrom) { return origsc->MatrixFrom(ifrom); }

#endif


		float Curve() { return curve; }

		// Transform to and from internal space
		Point3 PointTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 PointFrom(const Point3& p, RefFrame ifrom) { return p; } 
		Point3 VectorTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 VectorFrom(const Point3& p, RefFrame ifrom) { return p; } 
		SCTexture(){ doMaps = TRUE; 	curve = 0.0f;	dp = Point3(0.0f,0.0f,0.0f); }
	};


float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *data)
{
	if(data){
		SCTexture *texsc = new SCTexture(); //Might slow it down a bit. Can be moved out!
		texsc->duv.x= 0.f;
		texsc->duv.y= 0.f;
		texsc->uv.x= u;
		texsc->uv.y= v;
		ShadeContext *sc=(ShadeContext*)data;
		Texmap *t=(Texmap*)texmap;
		float value = t->EvalMono(*texsc);
		delete texsc;
		return value;
	}

	return 1.f;
}

float tl_eval_texmap_mono(void *texmap, void *data)
{
	if(data){
		ShadeContext *sc=(ShadeContext*)data;
		Texmap *t=(Texmap*)texmap;
		return t->EvalMono(*sc);
	}
	return 1.f;
}

tlColor tl_eval_texmap_color(void *texmap, void *data)
{
    tlColor ret = {1.f,1.f,1.f};
	if(data){
		ShadeContext *sc=(ShadeContext*)data;
		Texmap *t=(Texmap*)texmap;
		Point3 c=t->EvalColor(*sc);
		ret.r=c.x; ret.g=c.y; ret.b=c.z;
	}
    return ret;
}
