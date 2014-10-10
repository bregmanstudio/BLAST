#include "flext.h"
#include "SoundFile.h"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif

#include "SoundSpotter.h"

// The driver layer handles all the UI, State, I/O and buffer storage

class soundspotter: public flext_dsp {

  FLEXT_HEADER(soundspotter, flext_dsp)

    public:  
  soundspotter();
  ~soundspotter();

  
  void m_stop(int argc,const t_atom *argv);
  void m_spot(int argc,const t_atom *argv);
  void m_thru(int argc,const t_atom *argv);
  void m_liveSpot(int argc,const t_atom *argv);
  void m_extract(int argc,const t_atom *argv);
  void m_milt(int argc,const t_atom *argv);
  void m_set(int argc,const t_atom *argv);
  void m_sound(int argc,const t_atom *argv);
  void m_dump(int argc,const t_atom *argv);
  void m_load(int argc,const t_atom *argv);
  void m_float1(float f);
  void m_float2(float f);
  void m_floatLoBasis(float f);
  void m_floatBasisWidth(float f);
  void m_floatRadius(float r);
  void m_floatLoK(float f);
  void m_floatHiK(float f);
  void m_floatEnvFollow(float f);  
  void m_signal(int n, t_sample *const *in, t_sample *const *out);


 private:
  SoundSpotter* SS;
  SoundFile* aSound;

  // Buffer handlers
  const t_symbol *bufname; // PD internal source buffer name
  buffer *buf;             // PD internal buf pointer
  void flushBufs();
  void Clear();
  bool Check();

  FLEXT_CALLBACK_V(m_stop) // wrapper for method m_stop (with variable argument list)
    FLEXT_CALLBACK_V(m_liveSpot) // wrapper for method m_liveSpot (with variable argument list)
    FLEXT_CALLBACK_V(m_extract) // wrapper for method m_extract (with variable argument list)
    FLEXT_CALLBACK_V(m_milt) // wrapper for method m_milt (with variable argument list)
    FLEXT_CALLBACK_V(m_spot) // wrapper for method m_spot (with variable argument list)
    FLEXT_CALLBACK_V(m_thru) // wrapper for method m_thru (with variable argument list)
    FLEXT_CALLBACK_V(m_dump) // wrapper for method m_dump (with variable argument list)
    FLEXT_CALLBACK_V(m_load) // wrapper for method m_load (with variable argument list)
    FLEXT_CALLBACK_V(m_set) // wrapper for method m_set (with variable argument list)
    FLEXT_CALLBACK_V(m_sound) // wrapper for method m_sound (with variable argument list)
    FLEXT_CALLBACK_F(m_float1)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_float2)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_floatLoBasis)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_floatBasisWidth)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_floatRadius)  // callback for method "m_float" Radius
    FLEXT_CALLBACK_F(m_floatLoK)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_floatHiK)  // callback for method "m_float" (with one float argument)
    FLEXT_CALLBACK_F(m_floatEnvFollow) // callback for method "m_float" (with one float argument)
    
    // Before we can run our SoundSpotter-class in PD, the object has to be registered as a
    // PD object. Registering is made easy with the FLEXT_NEW_* macros defined
    // in flext.h. For tilde objects without arguments call:
    
};
