#version 460

vec2 uv;
vec2 R;

out vec4 C;

layout(binding = 3) uniform sampler2D comp_tex;
layout(binding = 4) uniform sampler2D back_tex;
uniform float u_t;
uniform bool reaction_diffusion;

const float BEAT_DUR=60./90.;
const float PI=acos(-1.);
const float TAU=PI*2.;

vec2 U;

const float pi=acos(-1.);
const float tau=pi*2.;

mat2 rot_(float a) {return mat2(cos(a),-sin(a),sin(a),cos(a));}
vec3 hash( uvec3 x )
{
    x = ((x>>8U)^x.yzx)*1103773245U;
    x = ((x>>8U)^x.yzx)*1103773245U;
    x = ((x>>8U)^x.yzx)*1103773245U;

    return vec3(x)*(1.0/float(0xffffffffU));
}

vec3 noise(vec3 st)
{
    uvec3[8] ids;
    ids[0]=uvec3(round(abs(st)+vec3(-.5,-.5,-.5))); //bl front
    ids[1]=uvec3(round(abs(st)+vec3( .5,-.5,-.5))); //br front
    ids[2]=uvec3(round(abs(st)+vec3(-.5, .5,-.5))); //tl front
    ids[3]=uvec3(round(abs(st)+vec3( .5, .5,-.5))); //tr front
    ids[4]=uvec3(round(abs(st)+vec3(-.5,-.5, .5))); //bl back
    ids[5]=uvec3(round(abs(st)+vec3( .5,-.5, .5))); //br back
    ids[6]=uvec3(round(abs(st)+vec3(-.5, .5, .5))); //tl back
    ids[7]=uvec3(round(abs(st)+vec3( .5, .5, .5))); //tr back

    vec3 st_f = fract(abs(st));
    st_f = smoothstep(0.,1.,st_f);

    return mix(
    mix(
    mix(hash(ids[0]),hash(ids[1]),st_f.x), //bottom front
    mix(hash(ids[2]),hash(ids[3]),st_f.x), //top    front
    st_f.y
    ), //front
    mix(
    mix(hash(ids[4]),hash(ids[5]),st_f.x), //bottom back
    mix(hash(ids[6]),hash(ids[7]),st_f.x), //top    back
    st_f.y
    ), //back
    st_f.z
    )*2.-1.;
}
vec3 cyclic(vec3 p, float pers, float lacu) {
  vec4 sum = vec4(0);

  for (int i = 0; i ++ < 5;) {
    //p *= rot;
    p.yz *= rot_(4.5);
    p.xy *= rot_(1.5);
    p += sin(p.zxy);
    sum += vec4(cross(cos(p), sin(p.yzx)), 1);
    sum /= pers;
    p *= lacu;
  }


  return sum.xyz / sum.w;
}
float stargaze(vec2 p) {
    float d = length(p);
    for (int k = 0; k < 5; k++) {
        float l = float[5](.6, .55, -1., .65, 0.)[k];
        vec2 n = sin((float(k) + vec2(0, 1.5)) * pi / 3.);
        d = min(d, length(clamp(dot(n, p), l, 1.) * n - p) - .16);
    }
    return d;
}

float kick_pat[20] = {1.,0.,0.,0.,0., 0.,0.,1.,0.,0., 0.,0.,0.,0.,0., 0.,0.,0.,0.,0.};
float sinease(float n) {
    return sin(-n*PI-PI/2.0)*0.5+0.5;
}

float expease(float n, float deg) {
    return n>0.0?1.0-exp(-n*deg):0.0;
}



float cube(vec3 p){
  p.zx *= rot_(u_t*2.77);
  p.x = abs(p.x);
  p.zy *= rot_(u_t*.33);
  p.x = abs(p.x);
  p.yx *= rot_(u_t*.23);
  p.y = abs(p.y);
  p.zy *= rot_(u_t*.43);
  p.x = abs(p.x);
  p.zx *= rot_(u_t*.97);
  p.x = abs(p.x);
  p.yx *= rot_(u_t*.63);
  p.y = abs(p.y);
  p.zy *= rot_(u_t*.13);
  p.x = abs(p.x);
  p.zx *= rot_(u_t*.87);
  p.x = abs(p.x);
  p.yx *= rot_(u_t*.55);
  return max(abs(p.x), max(abs(p.y), abs(p.z)));
}

float box(vec2 p) {
  vec2 q = abs(p);
  return max(q.x,q.y);
}

float heart(vec2 p) {
  vec2 q = abs(p);
  return mix(box(vec2(p.x-p.y,p.x+p.y))/sqrt(2.), length(q-(q.x*q.x+q.y*q.y)/(2.*(q.x+q.y))), step(0.,p.y)) - .5;
}

float scene(vec3 p){
  p.z -= 1.5;
  p.xz *= rot_(u_t);

  float c = cube(p)-1.;
  return c;
}

vec3 noScene(vec3 p){
  return normalize(vec3(scene(p+vec3(.0015,0,0))-scene(p-vec3(.0015,0,0)),
                        scene(p+vec3(0,.0015,0))-scene(p-vec3(0,.0015,0)),
                        scene(p+vec3(0,0,.0015))-scene(p-vec3(0,0,.0015))));
}

vec3 crystal( vec2 p) {
  vec3 ro=vec3(0,0,-1),
       rd=normalize(vec3(-p,1)),
       po=ro,
       co=vec3(.9),
       att=vec3(1),
       parallelLight=normalize(vec3(cos(u_t),0,sin(u_t)));
  
  int limitBounce=8;
  float dir=1.;
  float td,ld;
  for(int i=0; i<256; i++){
    po = ro+rd*td;
    
    ld = scene(po);
    ld *= dir;
    
    if(ld<.0015) {
      float IOR = 1.5;
      vec3 tempRd = refract(rd, noScene(po)*dir, dir>0.?IOR:1./IOR);
      
      vec3 cubeCo = vec3(0.5);
      // lambert
      if (dir<0.) {
        att *= exp(-.5*td)*cubeCo*1.;
      }
      att *= mix(vec3(1), cubeCo, .5);
      
      // surface looks at light
      co += exp((dot(-noScene(po), parallelLight)*.5-.5)*32.)*vec3(1)*2.;
      // looking generally at the light
      co += exp((dot(-rd, parallelLight)*.5-.5)*16.)*vec3(1)*.1;
      
      if(length(tempRd)>.0015) {
        rd = tempRd;
        dir *= -1.;
      }
      else {
        rd = reflect(rd, noScene(po)*dir);
      }
      
      td = 0.;
      ro = po;
      ro += rd*.0015*3.;
      
      limitBounce--;
      if (limitBounce<=0) {break;}
    }
    else {
      td += ld*.5;
    }
    
  }
  
  co *= att;
  return co;
}

vec3 logoscene(vec2 p) {
  vec2 pos=p;
  vec2 vel=vec2(0);
  for (int i=0; i<99; i++) {
    float t = u_t*.3;
    vec3 o = cyclic(vec3(t,0,0), .1, .1);
    vel += normalize(o.xy-p)*tanh(dot(vec2(cos(o.z*tau), sin(o.z*tau)), normalize(o.xy-p)))*9./length(o.xy-p);
    o = cyclic(vec3(0,t,0), .1, .1);
    vel += normalize(o.xy-p)*tanh(dot(vec2(cos(o.z*tau), sin(o.z*tau)), normalize(o.xy-p)))*5./length(o.xy-p);
    o = cyclic(vec3(0,0,t), .1, .1);
    vel += normalize(o.xy-p)*tanh(dot(vec2(cos(o.z*tau), sin(o.z*tau)), normalize(o.xy-p)))*4./length(o.xy-p);

    vel *= .9;
    pos += vel;
  }
  vec3 co=sin(vec3(0,1,2)+(length(vel)*.1))*.5+.5;
  
  float perc = clamp( u_t/BEAT_DUR/20.-13., 0., 1.);
  float logo = -stargaze(p*1.4);
  logo = logo>0.?1.:exp(logo*mix(100.,800.,cyclic(vec3(uv*3.,u_t),.1,9.).x*.5+.7)*pow(1.-perc,2.));
  co *= logo;
  return co;
}

float silly(vec2 p) {
  float v = 99.;
  for(float i=0.; i<6.; i++) {
    float sides = sin(u_t*.47)*1.5+4.5;
    vec2 q = p*rot_(tau*i/sides);
    v = min(v, q.x+sin(q.y)*(sin(u_t*.7)*.5+.5)+1.);
  }
  return abs(v-.5)-.05;
}

vec2 norSilly(vec2 p) {
  return normalize(vec2(
    silly(p+vec2(.0015,0.))-silly(p-vec2(.0015,0.)),
    silly(p+vec2(0.,.0015))-silly(p-vec2(0.,.0015))
  ));
}

float rod(vec2 p, float len) {
  float fun = p.x;
  p.x = abs(p.x);
  p.x-=len/2.;
  return (p.x>0.?max(abs(p.x),abs(p.y)):abs(p.y));
}

float arrow(vec2 p, float len) {
  float stem = rod(p-vec2(len/2.+.2,0), len);
  float head = min(rod(p*rot_(tau/8.)-vec2(.5,0), 1.), rod(p*rot_(-tau/8.)-vec2(.5,0), 1.));
  return min(stem, head);
}

float crawl(vec2 p) {
  return min(arrow(vec2(-p.x+1.5,p.y)*1.25,1.5)-.3,
  heart(p+vec2(1.0,0.)));
}

vec3 inkfuck(vec2 p) {
  vec3 co=vec3(0);

  // float mt = floor(u_t/BEAT_DUR)+expease(fract(u_t/BEAT_DUR), 2.);

  float kicktime = 0.;
  for (int i=0; i<20; i++) {kicktime += kick_pat[i];}
  kicktime *= floor(u_t/BEAT_DUR/5.);
  for (int i=0; i<20; i++) {
    int curr = int(mod(floor(u_t*4./BEAT_DUR),5.*4.));
    if (i>curr) {break;}
    kicktime += kick_pat[i]*expease(float(curr-i)+fract(u_t*4./BEAT_DUR),1.5);
  }
  
  vec2 focalEnd = vec2(-1,-.5);
  float prog = expease(u_t/BEAT_DUR/5.-8.25*4.,3.)*9.25-8.25;
  vec2 focal = focalEnd*max(0.,min(1.,prog));

  vec2 rotato = p * rot_(u_t*.1+kicktime*.3);
  vec2 po=rotato, ve=vec2(0);
  float ld=0.;
  for (int i=0; i<32; i++) {
    vec2 q = po*2.*rot_(float(i)*.06-cyclic(vec3(-kicktime*.4+length(p-focal)*1.7),5.,1.).x*.5);
    ld = silly(q);
    co += vec3(length(po-rotato))*.1;
    ve -= norSilly(q)*ld;
    po += ve;
    ve *= .5;
  }

  co = mix(co,vec3(1),step(arrow((p-focalEnd*prog)*2.*rot_(atan(focalEnd.y, -focalEnd.x)), 6.), .2));

  co = vec3(1)*pow(vec3(1.-exp(-co.r*.5)), vec3(6.));

  return co;
}

float fracScene(vec2 p) {
  // p += vec2(+.25);
  return -crawl(p*.2+vec2(3.-(u_t/BEAT_DUR/20.-10.)*2.,0.));
}

vec3 frac(vec2 p) {
  vec2 ip = floor(p*4.),
       fp = fract(p*4.);
  int layer = 3;

  float r = fracScene(ip+.5);
  if (r.x>.0) {
    layer--;
    ip += floor(fp*2.)/2.,
    fp = fract(fp*2.);
    r = fracScene(ip+.25);
    if (r.x>.1) {
      layer--;
      ip += floor(fp*2.)/4.,
      fp = fract(fp*2.);
      r = fracScene(ip+.125);
      if (r.x>.15) {
        layer--;
        ip += floor(fp*2.)/8.,
        fp = fract(fp*2.);
        r = cyclic(vec3(ip,0), 1., 5.).x;
      }
    }
  }

  float val = 1.;
  fp = mix(fp, vec2(1.-fp.x,fp.y), mod(ip.x*pow(2.,float(3-layer))+ip.y*pow(2.,float(3-layer)),2.));
  {
    vec2 q=1.-fp.x>fp.y?fp:vec2(1.)-fp;
    float d=step(length(q), .5)*(cos(length(q)*tau*pow(2.,float(layer)))*.5+.5);
    val = min(val, d);
  }
  {
    vec2 q=fp.x>fp.y?vec2(1.-fp.x,fp.y):vec2(fp.x,1.-fp.y);
    float d=step(length(q), .5);
    val = mix(val, cos(length(q)*tau*pow(2.,float(layer)))*.5+.5, d);
  }

  return pow(vec3(sin(-val*3.)*.5+.5),vec3(1.+sin(length(p)*2.-2.*u_t/BEAT_DUR)*.35));
}

float smin( float a, float b, float k )
{
    k *= 1.0;
    float r = exp2(-a/k) + exp2(-b/k);
    return -k*log2(r);
}

float rodr(vec2 p, float len, float slant){
  float fun = p.x*slant;
  p.x = abs(p.x);
  p.x-=len/2.;
  return (p.x>0.?length(p):abs(p.y))+fun;
}

vec3 floaters(vec2 p) {
  vec3 co=vec3(.9);

  p *= 2.;
  
  float acc = 999.;
  for (int i=0; i<36; i++) {
    float ind = float(i)+u_t*.3;
    
    vec2 shift = noise(vec3(ind*.2)+vec3(98.,48.,0.)).xy*30.;
    vec2 shiftF = abs(fract(shift)-.5)*2.;
    shift = mix(shift, round(shift), vec2(expease(shiftF.x,2.), expease(shiftF.y,2.)));
    float ang = noise(vec3(ind*.2)).z*TAU*.6;
    ang += (floor(ind)+expease(fract(ind),6.))*.5;
    
    float r = rodr((p*16.-shift)*rot_(ang), 3.,sin(ind)*.3);
    acc = smin(acc,abs(r-2.)+.4,.3);
  }

  acc = smin(acc, (-.03-abs(crawl(p/1.9+vec2(3.-(u_t/BEAT_DUR/20.-10.)*2.,0.))))*-16., .3);
  
  co -= min(acc+.26,1.)*.85+(noise(vec3(p*3.,u_t)).z*.5+.5)*.2+.1;

  return co;
}

float sh_crawl(vec2 p) {
  return crawl(p+vec2(3.-(u_t/BEAT_DUR/20.-9.)*2.,0.));
}

vec3 shadow(vec2 p) {
  vec3 co = vec3(1);

  float td=0.,ld=0.;
  vec2 light=vec2(3.)*rot_(u_t);
  vec2 po=p,rd=normalize(light-po);
  const vec2 ep=vec2(.0015,0);
  for(int i=0; i<128; i++) {
    po = p+rd*td;

    if (length(po-light)<ep.x) {
      break;
    }

    vec2 no = normalize(vec2(
      sh_crawl(po-ep.xy)-sh_crawl(po+ep.xy),
      sh_crawl(po-ep.yx)-sh_crawl(po+ep.yx)
    ));

    ld = max(0.,sh_crawl(po));
    co = min(co, vec3(pow(ld*16.+td*.01, .1)));

    if (td>999.) {
      break;
    }
    td += min(ld*.5, length(light-po));
  }
  if (sh_crawl(p)<0.) {
    co = vec3(1);
  }

  return co;
}

vec3 liquid(vec2 p) {
  float a = crawl(p+vec2(3.-(u_t/BEAT_DUR/20.-9.)*2., 0));
  a = smoothstep(a, a+.05, 0.);
  float t=floor(u_t/BEAT_DUR)+expease(fract(u_t/BEAT_DUR), 2.)-a;
  vec3 co = vec3(sin(vec3(0,.1,.2)+cyclic(vec3(p*2.,t), 1., 1.75-a*.15).x)*.5+.5);
  return pow(co, vec3(2.));
}

const vec2 RDSAMP = vec2(14.,7.);

// modified from https://www.shadertoy.com/view/XtdSDn
vec2 convolve(vec2 _uv) {
  vec2 result = vec2(0.0);
  for (float dx = -RDSAMP.x; dx <= RDSAMP.x; dx++) {
    for (float dy = -RDSAMP.x; dy <= RDSAMP.x; dy++) {
      vec2 d = vec2(dx, dy);
      float dist = length(d)-expease(1.-fract(u_t/BEAT_DUR), 2.)*1.;
      vec2 offset = d / R;
      vec2 samplepos = fract(_uv + offset);
      float weight = texture(back_tex, samplepos).x;
      result.x += weight * step(RDSAMP.y, dist) * (1.0-step(RDSAMP.x, dist));	
      result.y += weight * (1.0-step(RDSAMP.y, dist));
    }
  }
  return result;
}

float reaction(vec2 d) {
  return mix(mix(.69,-0.65,smoothstep(0.,1.,d.x*1.)), 
             mix(-.09,-.09,smoothstep(0.,1.,d.x*1.)), smoothstep(0.,1.,d.y*1.));
}

void main() {
	R = vec2(textureSize(comp_tex, 0));
	uv = gl_FragCoord.xy/R;
	uv -= 0.5;
	uv *= 2.0;
	U = gl_FragCoord.xy;

  vec3 comp=texture(comp_tex,U/R).xyz;
  //seed = uint(U.y*R.x)+uint(U.x);
  float isFront = 0.;
  vec2 p=vec2(uv.x*R.x/R.y, uv.y);
  
  if (reaction_diffusion) {
    vec3 b = texture(back_tex, U/R).rgb;
    vec2 nconv = convolve(U/R)/vec2(pi*RDSAMP.x*RDSAMP.x, pi*RDSAMP.x*RDSAMP.x-pi*RDSAMP.y*RDSAMP.y);
    b.r = b.r+.3*(reaction(nconv)*2.-1.);
    b.gb = nconv;
    C=clamp(vec4(
      b+vec3(
        max(0.,(.1-abs(crawl((p+vec2(3.-(u_t/BEAT_DUR/20.-10.)*2., 0))*1.)))*10.),
      0,0)*1.+(
        cyclic(vec3(p,floor(u_t/BEAT_DUR)+expease(fract(u_t/BEAT_DUR),2.)),1.,1.8)
      )*.2-.05,
      1),vec4(-1),vec4(1));
    return;
  }

  vec3 co;
  
  if (u_t/BEAT_DUR/20.>7. && u_t/BEAT_DUR/20.<8.73) {
    isFront = smoothstep(7.,8.,u_t/BEAT_DUR/20.);
    co = inkfuck(p);
    co = mix(co, vec3(1)-co, comp);
  }
  else if (u_t/BEAT_DUR/20.>8.73 && u_t/BEAT_DUR/20.<9.) {
    isFront = 1.;
    co = crystal(p*mix(20., 1., smoothstep(8.5,9.,u_t/BEAT_DUR/20.)));
  }
  else if (u_t/BEAT_DUR/20.>9. && u_t/BEAT_DUR/20.<13.) {
    float inner = u_t/BEAT_DUR/20.-9.;
    if (inner<.5) {
      isFront = 1.;
      co = pow(shadow(p),vec3(1.2));
    } else if (inner<1.) {
      isFront = 1.;
      co = liquid(p);
    } else if (inner<2.) {
    } else if (inner<2.5) {
      isFront = 1.;
      co = floaters(p);
      co = pow(co,vec3(.454545));
    } else if (inner<3.) {
    } else if (inner<3.5) {
      isFront = 1.;
      vec3 rd = texture(back_tex, U/R).rgb;
      co = vec3(sinease(rd.r*.2+.5-rd.g*.2));
      co *= .6;
    } else {
      isFront = 1.;
      co = frac(p);
    }
  }
  else if (u_t/BEAT_DUR/20.>13.) {
    isFront = 1.;
    co = logoscene(p);
    co = pow(co,vec3(.454545));
    // co = step(pow(sin(p.x-u_t*3.)*.5+.5,19.)*.05, vec3(crawl(p)));
  }

  vec3 finalColor = mix(comp,co,isFront);
  finalColor = mix(vec3(.002,.01,.003),vec3(.95,1.2,1.),finalColor);
  C=vec4(finalColor,1);
}