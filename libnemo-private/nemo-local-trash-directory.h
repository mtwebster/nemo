/*
 * Custom GObject NEMO-Header
 */

/* inclusion guard */
#ifndef __NEMO_LOCAL_TRASH_DIRECTORY_H__
#define __NEMO_LOCAL_TRASH_DIRECTORY_H__

/*
 * Type macros.
 */
#define NEMO_TYPE_LOCAL_TRASH_DIRECTORY \
  (nemo_local_trash_directory_get_type())
#define NEMO_LOCAL_TRASH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), NEMO_TYPE_LOCAL_TRASH_DIRECTORY, NEMOLOCAL_TRASH_DIRECTORY))
#define NEMO_IS_LOCAL_TRASH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_LOCAL_TRASH_DIRECTORY))
#define NEMO_LOCAL_TRASH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), NEMO_TYPE_LOCAL_TRASH_DIRECTORY, \
                           NEMOLOCAL_TRASH_DIRECTORYClass))
#define NEMO_IS_LOCAL_TRASH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), NEMO_TYPE_LOCAL_TRASH_DIRECTORY))
#define NEMO_LOCAL_TRASH_DIRECTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), NEMO_TYPE_LOCAL_TRASH_DIRECTORY, \
                             NEMOLOCAL_TRASH_DIRECTORYClass))

typedef struct _NEMOLOCAL_TRASH_DIRECTORY NEMOLOCAL_TRASH_DIRECTORY;
typedef struct _NEMOLOCAL_TRASH_DIRECTORYPrivate NEMOLOCAL_TRASH_DIRECTORYPrivate;
typedef struct _NEMOLOCAL_TRASH_DIRECTORYClass NEMOLOCAL_TRASH_DIRECTORYClass;

struct _NEMOLOCAL_TRASH_DIRECTORY
{
  GObject parent_instance;

  /* instance members */
    
  /*< private >*/
  NEMOLOCAL_TRASH_DIRECTORYPrivate *priv;
};

struct _NEMOLOCAL_TRASH_DIRECTORYClass
{
  GObjectClass parent_class;

  /* class members */
};

GType nemo_local_trash_directory_get_type (void);

/*
 * Method definitions.
 */

NEMOLOCAL_TRASH_DIRECTORY *nemo_local_trash_directory_new (void);
void nemo_local_trash_directory_do_action (NEMOLOCAL_TRASH_DIRECTORY *);

#endif /* __NEMO_LOCAL_TRASH_DIRECTORY_H__ */
