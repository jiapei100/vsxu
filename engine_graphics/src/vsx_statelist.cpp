#include "vsx_statelist.h"


void vsx_statelist::init_current(vsx_engine *vxe_local, state_info* info) {
  //title_timer = 5.0f;
  if (vxe_local == 0)
  {
    vxe_local = new vsx_engine(own_path);
    vxe_local->dump_modules_to_disk = false;
    vxe_local->no_client_time = true;
    vxe_local->init();
    vxe_local->start();
    (*state_iter).engine = vxe_local;
#ifdef VSXU_DEBUG
    printf("loading state: %s\n", (*state_iter).state_name.c_str());
#endif
    vxe_local->load_state((*state_iter).state_name);

  } else
  {
    if (info->need_reload) {
      vxe_local->unload_state();
      vxe_local->load_state(info->state_name);
      info->need_reload = false;
    }
    vxe_local->g_timer.start();
  }
}

void vsx_statelist::toggle_randomizer() 
{
  randomizer = !randomizer;
}

bool vsx_statelist::get_randomizer_status()
{
  return randomizer;
}

void vsx_statelist::set_randomizer(bool status)
{
  randomizer = status;
}

void vsx_statelist::next_state()
{
  if ((*state_iter).engine != vxe) return;
  ++state_iter;
  if (state_iter == statelist.end()) state_iter = statelist.begin();
  init_current((*state_iter).engine, &(*state_iter));
  transition_time = 2.0f;
}
void vsx_statelist::prev_state()
{
  if ((*state_iter).engine != vxe) return;
  if (state_iter == statelist.begin()) state_iter = statelist.end();
  --state_iter;
  init_current((*state_iter).engine, &(*state_iter));
  transition_time = 2.0f;
}

vsx_string vsx_statelist::state_loading()
{
  if (transition_time > 1.0f) {
    return (*state_iter).state_name;
  }
  return "";
}

void vsx_statelist::inc_speed() 
{
  (*state_iter).speed *= 1.04f;
  if ((*state_iter).speed > 16.0f) (*state_iter).speed = 16.0f;
  vxe->set_speed((*state_iter).speed);
}

void vsx_statelist::dec_speed() 
{
  (*state_iter).speed *= 0.96f;
  if ((*state_iter).speed < 0.0f) (*state_iter).speed = 0.0f;
  vxe->set_speed((*state_iter).speed);
}


float vsx_statelist::get_speed()
{
  return (*state_iter).speed;
}


void vsx_statelist::inc_amp() 
{
  (*state_iter).fx_level+=0.05f;
  if ((*state_iter).fx_level > 16.0f) (*state_iter).fx_level = 16.0f;
#if defined(__linux__)
  vsx_string fxlf = config_dir+"/"+(*state_iter).state_name_suffix.substr(visual_path.size()+1 , (*state_iter).state_name_suffix.size())+"_fx_level";
#ifdef VSXU_DEBUG
  printf("fx level file: %s\n", fxlf.c_str() );
#endif
  FILE* fxfp = fopen( fxlf.c_str(), "w");
  if (fxfp)
  {
    fputs(f2s((*state_iter).fx_level).c_str(), fxfp);
    fclose(fxfp);
  }
#endif
  vxe->set_amp((*state_iter).fx_level);
  fx_alpha = 5.0f;
}

void vsx_statelist::dec_amp()
{
  (*state_iter).fx_level-=0.05f;
  if ((*state_iter).fx_level < 0.1f) (*state_iter).fx_level = 0.1f;
  vxe->set_amp((*state_iter).fx_level);
#if defined(__linux__)
  vsx_string fxlf = config_dir+"/"+(*state_iter).state_name_suffix.substr(visual_path.size()+1 , (*state_iter).state_name_suffix.size())+"_fx_level";
  FILE* fxfp = fopen( fxlf.c_str(), "w");
  if (fxfp)
  {
    fputs(f2s((*state_iter).fx_level).c_str(), fxfp);
    fclose(fxfp);
  }
#endif
  fx_alpha = 5.0f;
}

float vsx_statelist::get_fx_level()
{
  return (*state_iter).fx_level;
}

void vsx_statelist::start()
{
  vxe->start();
  vxe->load_state((*state_iter).state_name);
}
void vsx_statelist::stop()
{
  for (std::vector<state_info>::iterator it = statelist.begin(); it != statelist.end(); ++it)
  {
    if ((*it).engine)
    (*it).engine->stop();
    (*it).need_reload = true;
    //(*it).need_stop = true;
  }
  vxe->unload_state();
  vxe->stop();
}

vsx_string vsx_statelist::get_meta_visual_filename()
{
  return (*state_iter).state_name;
}
vsx_string vsx_statelist::get_meta_visual_name()
{
  return (*state_iter).engine->meta_fields[0];
}
vsx_string vsx_statelist::get_meta_visual_creator()
{
  return (*state_iter).engine->meta_fields[1];
}
vsx_string vsx_statelist::get_meta_visual_company()
{
  return (*state_iter).engine->meta_fields[2];
}



/*void vsx_statelist::toggle_fullscreen() 
{
  vsx_texture aa;
  aa.unload_all_active();
  bool a = app_get_fullscreen(0);
  //vxe->stop();
  for (std::vector<state_info>::iterator it = statelist.begin(); it != statelist.end(); ++it)
  {
    if ((*it).engine)
    (*it).engine->stop();
    (*it).need_reload = true;
    //(*it).need_stop = true;
  }
  //(*state_iter).need_stop = false;
  vxe->unload_state();
  vxe->stop();
  app_set_fullscreen(0,!a);
  vxe->start();
  vxe->load_state((*state_iter).state_name);
  // todo, clear out the other engines as well
}*/

void vsx_statelist::random_state() {
  if ((*state_iter).engine != vxe) return;
  int steps = rand() % statelist.size();
  while (steps) {
    ++state_iter;
    if (state_iter == statelist.end()) state_iter = statelist.begin();
    --steps;
  }
  if ((*state_iter).engine == vxe) { random_state(); return; }
  init_current((*state_iter).engine, &(*state_iter));
  transition_time = 2.0f;
}

void vsx_statelist::render() 
{
  if (render_first) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    /*      if (vsx_string((char*)glGetString(GL_VENDOR)) == vsx_string("ATI Technologies Inc."))
    {
      int r = 512;
      tex1.init_buffer(r,r);
      tex_to.init_buffer(r, r);
    } else*/
    {
      tex1.init_buffer(viewport[2], viewport[3]);
      tex_to.init_buffer(viewport[2], viewport[3]);
    }

    get_files_recursive(own_path+"visuals_faders", &fader_file_list,"",".svn CVS");
    for (std::list<vsx_string>::iterator it = fader_file_list.begin(); it != fader_file_list.end(); ++it) {
#ifdef VSXU_DEBUG
      printf("initializing fader %s\n", (*it).c_str());
#endif
      lvxe = new vsx_engine(own_path);
      lvxe->dump_modules_to_disk = false;
      lvxe->init();
      lvxe->start();
      lvxe->load_state(*it);
      faders.push_back(lvxe);
      fade_id = 0;
    }
    init_current((*state_iter).engine, &(*state_iter));
    vxe = (*state_iter).engine;
    cmd_in = &(*state_iter).cmd_in;
    cmd_out = &(*state_iter).cmd_out;
    transitioning = false;

    render_first = false;
  }

  //printf("r2");
  if ((*state_iter).engine != vxe) // change is on the way
  {
    //printf("r3");
    //printf("rendering tex_to %f\n", transition_time);
    tex_to.begin_capture();
      if ((*state_iter).engine)
      {
        //printf("\n%s\n", (*state_iter).state_name.c_str());
        //printf("r31");
        (*state_iter).engine->process_message_queue(&(*state_iter).cmd_in,&(*state_iter).cmd_out);
        //printf("r32");
        (*state_iter).engine->render();
        //printf("r33");
      }
      glColorMask(false, false, false, true);
      glClearColor(0,0,0,1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glColorMask(true, true, true, true);
    tex_to.end_capture();
    //printf("r34");
    //printf("vme2->modules_loaded: %d\n",(*state_iter).engine->modules_loaded);
    //printf("vme2->modules_left_to_load: %d\n",(*state_iter).engine->modules_left_to_load);
    //printf("vme2->internal_msg_size: %d\n",(*state_iter).engine->commands_internal.count());
    if (
      (*state_iter).engine->modules_left_to_load == 0 &&
      (*state_iter).engine->commands_internal.count() == 0 &&
      transition_time > 1.0f
    )
    {
      //printf("setting transition time to 1\n");
      transition_time = 1.0f;
      timer.start();
      fade_id = rand() % (faders.size());
    }

    if (transition_time <= 0.0)
    {
      //printf("transition time < 0.0\n");
      vxe = (*state_iter).engine;
      cmd_in = &(*state_iter).cmd_in;
      cmd_out = &(*state_iter).cmd_out;
      transitioning = false;
      transition_time = 2.0f;
    if (cmd_out && cmd_in)
    {
      if (vxe)
      {
        vxe->process_message_queue(cmd_in,cmd_out);
      }
      cmd_out->clear();
    }
    if (vxe)
    {
      vxe->render();
    }
    } else
    {
      if (cmd_out && cmd_in)
      {
        if (vxe)
        {
          vxe->process_message_queue(cmd_in,cmd_out);
        }
        cmd_out->clear();
      }
      tex1.begin_capture();
        if (vxe)
        {
          vxe->render();
        }
        glColorMask(false, false, false, true);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColorMask(true, true, true, true);

      tex1.end_capture();
      //printf("a3 %d\n",faders.size());
      //printf("num modules: %d\n", faders[0]->get_num_modules());
      //vsx_comp* comp = faders[0]->get_by_name("visual_fader");
      vsx_module_param_texture* param_t_a = (vsx_module_param_texture*)faders[fade_id]->get_in_param_by_name("visual_fader", "texture_a_in");
      //printf("a4\n");
      vsx_module_param_texture* param_t_b = (vsx_module_param_texture*)faders[fade_id]->get_in_param_by_name("visual_fader", "texture_b_in");
      vsx_module_param_float* param_pos = (vsx_module_param_float*)faders[fade_id]->get_in_param_by_name("visual_fader", "fade_pos_in");
      vsx_module_param_float* fade_pos_from_engine = (vsx_module_param_float*)faders[fade_id]->get_in_param_by_name("visual_fader", "fade_pos_from_engine");
      //printf("a4\n");
      faders[fade_id]->process_message_queue(&l_cmd_in, &l_cmd_out);
      l_cmd_out.clear();
      if (param_t_a && param_t_b && param_pos && fade_pos_from_engine)
      {
        //printf("rendering fader\n");
        param_t_a->set_p(tex1);
        param_t_b->set_p(tex_to);
        fade_pos_from_engine->set(1.0f);
        float t = transition_time;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
        param_pos->set(1.0-t);
        //cmd_out->clear();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        faders[fade_id]->render();
      }
      //
      if (transition_time <= 1.0f)
      {
        transition_time -= timer.dtime();
      }
    }
  } else
  {
    if (cmd_out && cmd_in)
    {
      //printf("num in commands: %d\n",cmd_in->commands.size());
      //printf("num ou commands: %d\n",cmd_out->commands.size());
      vxe->process_message_queue(cmd_in, cmd_out);
      cmd_out->clear();
    }
    vxe->render();
    if (randomizer)
    {
      randomizer_time -= vxe->engine_info.real_dtime;
      //printf("%f\n", randomizer_time);
      if (randomizer_time < 0.0f)
      {
        random_state();
        randomizer_time = (float)(rand()%1000)*0.001f*15.0f+10.0f;
      }
    }
  }
}

 

void vsx_statelist::load_fx_levels_from_user()
{
#if PLATFORM == PLATFORM_LINUX
  struct stat st;

  char* home_dir = getenv ("HOME");
  config_dir = home_dir;
  config_dir += "/.vsxu_player";
  if (access(config_dir.c_str(),0) != 0) mkdir(config_dir.c_str(),0700);

  for (std::vector<state_info>::iterator it = statelist.begin(); it != statelist.end(); it++)
  {
    state_info state = (*it);
    // read fx level files
    vsx_string fxlf = config_dir+"/"+state.state_name_suffix.substr(visual_path.size()+1 , state.state_name_suffix.size())+"_fx_level";
#ifdef VSXU_DEBUG
    printf("fx level file: %s\n", fxlf.c_str() );
#endif
    if ( stat( fxlf.c_str(), &st) != 0 )
    {
      // no fx level file
      FILE* fpfx = fopen(fxlf.c_str(), "w");
      if (fpfx)
      {
        vsx_string ff = "1.0";
        fputs(ff.c_str(), fpfx);
        fclose(fpfx);
      }
      state.fx_level = 1.0f;
    } else
    {
      // got the file
      FILE* fpfx = fopen(fxlf.c_str(), "r");
      char dest[256];
      fgets(dest, 256, fpfx);
      fclose(fpfx);
      vsx_string ff = dest;
      state.fx_level = s2f(ff);
    }
  }
#endif
}

void vsx_statelist::save_fx_levels_from_user()
{
#if PLATFORM == PLATFORM_LINUX
  struct stat st;

  char* home_dir = getenv ("HOME");
  config_dir = home_dir;
  config_dir += "/.vsxu_player";
  if (stat(config_dir.c_str(),&st) != 0)
  mkdir(config_dir.c_str(),0700);

  for (std::vector<state_info>::iterator it = statelist.begin(); it != statelist.end(); it++)
  {
    state_info state = (*it);
    // read fx level files
    vsx_string fxlf = config_dir+"/"+state.state_name_suffix.substr(visual_path.size()+1 , state.state_name_suffix.size())+"_fx_level";
#ifdef VSXU_DEBUG
    printf("fx level file: %s\n", fxlf.c_str() );
#endif
    if ( stat( fxlf.c_str(), &st) != 0 )
    {
      // no fx level file
      FILE* fpfx = fopen(fxlf.c_str(), "w");
      if (fpfx)
      {
        vsx_string ff = "1.0";
        fputs(ff.c_str(), fpfx);
        fclose(fpfx);
      }
      state.fx_level = 1.0f;
    } else
    {
      // got the file
      FILE* fpfx = fopen(fxlf.c_str(), "r");
      char dest[256];
      fgets(dest, 256, fpfx);
      fclose(fpfx);
      vsx_string ff = dest;
      state.fx_level = s2f(ff);
    }
  }
#endif
}


void vsx_statelist::init(vsx_string base_path)
{
  no_fbo_ati = false;
  if (vsx_string((char*)glGetString(GL_VENDOR)) == vsx_string("ATI Technologies Inc.")) no_fbo_ati = true;
  randomizer = true;
  srand ( time(NULL)+rand() );
  randomizer_time = (float)(rand()%1000)*0.001f*15.0f+10.0f;
  transition_time = 2.0f;
  message_time = -1.0f;
  fx_alpha = 0.0f;
  spd_alpha = 0.0f;
  render_first = true;
  transitioning = false;
  first = 2;
  show_progress_bar = false;
  vxe = 0;

  cmd_out = 0;
  own_path = base_path;
#ifdef VSXU_DEBUG
  printf("own path: %s\n", own_path.c_str() );
#endif
/*
  vsx_avector<vsx_string> parts;
  vsx_avector<vsx_string> parts2;
#if defined(__linux__)
  vsx_string deli = "/";
#else
  vsx_string deli = "\\";
#endif
  explode(own_path, deli, parts);
  for (unsigned long i = 0; i < parts.size()-1; ++i)
  {
    parts2.push_back(parts[i]);
  }
  own_path = implode(parts2,deli);
#if defined(__linux__)
  if (own_path.size()) own_path.push_back('/');
#else
  if (own_path.size()) own_path.push_back('\\');
#endif
  printf("own path: %s\n",own_path.c_str());
  //printf("argc: %d\n",argc);
*/
#if PLATFORM == PLATFORM_WINDOWS
  if (own_path.size())
  if (own_path[own_path.size()-1] != '\\') own_path.push_back('\\');
#else
  if (own_path[own_path.size()-1] != '/') own_path.push_back('/');
#endif
  visual_path = "visuals_player";

/*  if (app_argc > 1) {
    //printf("argv1: %s\n", app_argv[1]);
    if (vsx_string(app_argv[1]) == "-vd") {
      visual_path = vsx_string(app_argv[2]);
      //printf("changing visual path to: %s\n", visual_path.c_str() );
    }
  }
*/

  get_files_recursive(own_path+visual_path, &state_file_list,"","");
#ifdef VSXU_DEBUG
  printf("getting files recursive: %s\n", (own_path+visual_path).c_str() );
#endif
  for (std::list<vsx_string>::iterator it = state_file_list.begin(); it != state_file_list.end(); ++it) {
    state_info state;
    state.state_name = *it;
    state.state_name_suffix = state.state_name.substr(own_path.size(),state.state_name.size() - own_path.size() );
#ifdef VSXU_DEBUG
    printf("adding state %s\n",(*it).c_str());
#endif

    state.fx_level = 1.0f;
    state.engine = 0;
    statelist.push_back(state);
  }
  state_iter = statelist.begin();
  load_fx_levels_from_user();
}

vsx_statelist::vsx_statelist() 
{
};