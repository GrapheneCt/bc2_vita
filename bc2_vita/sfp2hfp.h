#ifndef __SFP2HFP_H__
#define __SFP2HFP_H__

#if defined(SYMT_HAS_PVR_PSP2_GLES1) || defined(SYMT_HAS_PVR_PSP2_GLES2)

void glTexEnvf_sfp(GLenum target, GLenum pname, int param)
{
	float fa1;

	fa1 = *(float *)(&param);
	glTexEnvf(target, pname, fa1);
}

void glFogf_sfp(GLenum pname, int param)
{
	float fa1;

	fa1 = *(float *)(&param);
	glFogf(pname, fa1);
}

void glClearDepthf_sfp(int depth)
{
	float fa1;

	fa1 = *(float *)(&depth);
	glClearDepthf(fa1);
}

void glDepthRangef_sfp(int fZNear, int fZFar)
{
	float fa1, fa2;

	fa1 = *(float *)(&fZNear);
	fa2 = *(float *)(&fZFar);
	glDepthRangef(fa1, fa2);
}

void glAlphaFunc_sfp(GLenum func, int ref)
{
	float fa1;

	fa1 = *(float *)(&ref);
	glAlphaFunc(func, fa1);
}

#endif

int powf_sfp(int a1, int a2)
{
	float fa1, fa2;
	int ires;

	fa1 = *(float *)(&a1);
	fa2 = *(float *)(&a2);
	float fres = powf(fa1, fa2);
	ires = *(int *)(&fres);

	return ires;
}

int64_t acos_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = acos(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t asin_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = asin(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t atan_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = atan(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t atan2_sfp(int64_t a1, int64_t a2)
{
	double fa1, fa2;
	int64_t ires;

	fa1 = *(double *)(&a1);
	fa2 = *(double *)(&a2);
	double fres = atan2(fa1, fa2);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t ceil_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = ceil(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t cos_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = cos(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t floor_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = floor(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t fmod_sfp(int64_t a1, int64_t a2)
{
	double fa1, fa2;
	int64_t ires;

	fa1 = *(double *)(&a1);
	fa2 = *(double *)(&a2);
	double fres = fmod(fa1, fa2);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t ldexp_sfp(int64_t a1, int a2)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = ldexp(fa1, a2);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t log_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = log(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t pow_sfp(int64_t a1, int64_t a2)
{
	double fa1, fa2;
	int64_t ires;

	fa1 = *(double *)(&a1);
	fa2 = *(double *)(&a2);
	double fres = pow(fa1, fa2);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t sin_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = sin(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t sqrt_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = sqrt(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

int64_t tan_sfp(int64_t a1)
{
	double fa1;
	int64_t ires;

	fa1 = *(double *)(&a1);
	double fres = tan(fa1);
	ires = *(int64_t *)(&fres);

	return ires;
}

#endif
