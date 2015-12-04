
class module_texture_load_jpeg : public vsx_module
{
  // in
  vsx_module_param_resource* filename_in;

  // out
  vsx_module_param_bitmap* bitmap_out;
  vsx_module_param_texture* texture_out;

  // internal
  vsx_texture<>* texture;

  // threading stuff
  void* pti_l;
  vsx_string<>current_filename;
  vsx_bitmap bitmap;
  int bitm_timestamp; // keep track of the timestamp for the bitmap internally
  volatile int               thread_state;
  bool              thread_working;
  pthread_t         worker_t;
  pthread_attr_t    worker_t_attr;
  int texture_timestamp;

  static void* jpeg_worker_v(void *ptr)
  {
    module_texture_load_jpeg* mod = (module_texture_load_jpeg*)ptr;

    CJPEGTest* cj = new CJPEGTest;
    vsx_string<>ret;
    if
    (
     !(cj
       ->LoadJPEG(
       mod->current_filename,
       ret,
       mod->engine->filesystem
     ))
    )
    {
      mod->message = "module||"+ret+"\n"+((module_texture_load_jpeg*)ptr)->current_filename;
      mod->thread_state = -1;
      delete cj;
      return 0;
    }
    mod->bitmap.width = cj->GetResX();
    mod->bitmap.height = cj->GetResY();

    // allocate data in the vsx_bitmap
    vsx_bitmap_32bt b_c = (mod->bitmap.width) * (mod->bitmap.height);

    unsigned char* rb = (unsigned char*)cj->m_pBuf;
    mod->bitmap.data[0] = malloc( sizeof(vsx_bitmap_32bt) * b_c );

    for (unsigned long i = 0; i < b_c; ++i)
    {
      ((vsx_bitmap_32bt*)mod->bitmap.data)[i] =
          0xFF000000 |
          rb[i*3+2] << 16 |
          rb[i*3+1] << 8 |
          rb[i*3];
    }
    delete cj;

    // memory barrier
    asm volatile("":::"memory");

    mod->thread_state = 2;

    return 0;
  }
  
public:

  int m_type;

  void module_info(vsx_module_info* info)
  {
    if (m_type == 0)
    {
      info->identifier = "bitmaps;loaders;jpeg_bitm_load";
      info->component_class = "bitmap";
    }
    else
    {
      info->identifier = "texture;loaders;jpeg_tex_load";
      info->component_class = "texture";
    }

    info->description =
      "Loads a JPEG image from\ndisk and outputs a \n"
      "- VSXu bitmap \n "
      "and\n "
      "- texture.\n"
      "Texture is only loaded when used.\n"
      "This is to preserve memory."
    ;

    info->in_param_spec =
      "filename:resource";

    info->out_param_spec =
      "texture:texture,bitmap:bitmap";
  }
  
  void declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters)
  {
    texture_timestamp = -1;
    loading_done = false;
    
  	filename_in = (vsx_module_param_resource*)in_parameters.create(VSX_MODULE_PARAM_ID_RESOURCE,"filename");
  	filename_in->set("");
  	current_filename = "";
  	
  	// out
  	bitmap_out = (vsx_module_param_bitmap*)out_parameters.create(VSX_MODULE_PARAM_ID_BITMAP,"bitmap");
    
    thread_state = 0;
    texture = 0x0;
    texture_out = (vsx_module_param_texture*)out_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"texture");
  }
  
  void run()
  {
    if (current_filename != filename_in->get())
    {
      if (thread_state == -1)
        message = "module||ok";

      // time to decode a new jpg
      if (thread_state == 1)
      {
        long* aa;
        pthread_join(worker_t, (void **)&aa);
      }

      if (!verify_filesuffix(filename_in->get(),"jpg"))
      {
     		filename_in->set(current_filename);
     		message = "module||ERROR! This is not a JPG image file!";
     		return;
      }
      message = "module||ok";
      
      current_filename = filename_in->get();
      pthread_attr_init(&worker_t_attr);
      thread_state = 1;
      pthread_create(&(worker_t), &(worker_t_attr), &jpeg_worker_v, (void*)this);
    }
    if (thread_state == 2)
    {
      pthread_join(worker_t, NULL);
      ++bitmap.timestamp;
      thread_state = 3;
      loading_done = true;
      bitmap_out->set(&bitmap);
    }
  }

  void output(vsx_module_param_abs* param)
  {
    if (param == (vsx_module_param_abs*)texture_out)
    {
      if (texture_timestamp != bitmap.timestamp)
      {
        if (!texture)
        {
          texture = new vsx_texture<>;
          texture->texture->init_opengl_texture_2d();
        }
        texture->texture->hint |= vsx_texture_gl::mipmaps_hint;
        vsx_texture_gl_loader::upload_bitmap_2d(texture->texture, &bitmap, true);
        texture_out->set(texture);
        texture_timestamp = bitmap.timestamp;
      }
    }
  }
  
  void on_delete()
  {
    if (thread_state == 1) 
      pthread_join(worker_t, 0);

    if (bitmap.data[0])
      free(bitmap.data[0]);

    if (texture)
    {
      texture->unload_gl();
      delete texture;
    }
  }
};

