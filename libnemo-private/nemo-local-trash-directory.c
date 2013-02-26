/*
 * Custom GObject NEMO-Header
 */

#include <glib-object.h>

#include "nemo_local_trash_directory.h"
#include "nemo_local_trash_directory_private.h"

#define NEMO_LOCAL_TRASH_DIRECTORY_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NEMO_TYPE_LOCAL_TRASH_DIRECTORY, \
                                NEMOLOCAL_TRASH_DIRECTORYPrivate)) 

static void nemo_local_trash_directory_class_init (NEMOLOCAL_TRASH_DIRECTORYClass *);
static void nemo_local_trash_directory_init (NEMOLOCAL_TRASH_DIRECTORY *);
static void nemo_local_trash_directory_constructed (GObject *);
static void nemo_local_trash_directory_dispose (GObject *);
static void nemo_local_trash_directory_finalize (GObject *);

G_DEFINE_TYPE (NEMOLOCAL_TRASH_DIRECTORY, nemo_local_trash_directory, G_TYPE_OBJECT)

static void
nemo_local_trash_directory_class_init (NEMOLOCAL_TRASH_DIRECTORYClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->constructed = nemo_local_trash_directory_constructed;
  gobject_class->dispose = nemo_local_trash_directory_dispose;
  gobject_class->finalize = nemo_local_trash_directory_finalize;

  g_type_class_add_private (klass, sizeof(NEMOLOCAL_TRASH_DIRECTORYPrivate));
}

static void
nemo_local_trash_directory_init (NEMOLOCAL_TRASH_DIRECTORY *self)
{
  NEMOLOCAL_TRASH_DIRECTORYPrivate *priv;

  self->priv = priv = NEMO_LOCAL_TRASH_DIRECTORY_GET_PRIVATE (self);

  /* initialize all public and private members to reasonable default values. */
}

static void
nemo_local_trash_directory_constructed (GObject *gobject)
{
  NEMOLOCAL_TRASH_DIRECTORY *self = NEMO_LOCAL_TRASH_DIRECTORY(gobject);
}

static void
nemo_local_trash_directory_dispose (GObject *gobject)
{
  NEMOLOCAL_TRASH_DIRECTORY *self = NEMO_LOCAL_TRASH_DIRECTORY(gobject);

  /* 
   * In dispose, you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a 
   * reference.
   */

  /* dispose might be called multiple times, so we must guard against
   * calling g_object_unref() on an invalid GObject.
   */

  /* Chain up to the parent class */
  G_OBJECT_CLASS(nemo_local_trash_directory_parent_class)->dispose(gobject);
}

static void
nemo_local_trash_directory_finalize (GObject *gobject)
{
  NEMOLOCAL_TRASH_DIRECTORY *self = NEMO_LOCAL_TRASH_DIRECTORY(gobject);

  /* Chain up to the parent class */
  G_OBJECT_CLASS(nemo_local_trash_directory_parent_class)->finalize(gobject);
}

NEMOLOCAL_TRASH_DIRECTORY *
nemo_local_trash_directory_new (void)
{
  return g_object_new(NEMO_TYPE_LOCAL_TRASH_DIRECTORY, NULL);
}

void
nemo_local_trash_directory_do_action (NEMOLOCAL_TRASH_DIRECTORY *self)
{
  g_return_if_fail (NEMO_IS_LOCAL_TRASH_DIRECTORY(self));

  /* do stuff here. */
}

