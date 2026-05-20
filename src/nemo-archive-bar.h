/* nemo-archive-bar.h: GtkInfoBar with Extract Selected / Extract All buttons.
 *
 * Appears above the file view when navigated into an x-nemo-archive: URI.
 */

#ifndef NEMO_ARCHIVE_BAR_H
#define NEMO_ARCHIVE_BAR_H

#include <gtk/gtk.h>
#include "nemo-view.h"

#define NEMO_TYPE_ARCHIVE_BAR         (nemo_archive_bar_get_type ())
#define NEMO_ARCHIVE_BAR(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_TYPE_ARCHIVE_BAR, NemoArchiveBar))
#define NEMO_IS_ARCHIVE_BAR(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_ARCHIVE_BAR))

typedef struct _NemoArchiveBar        NemoArchiveBar;
typedef struct _NemoArchiveBarClass   NemoArchiveBarClass;
typedef struct _NemoArchiveBarPrivate NemoArchiveBarPrivate;

struct _NemoArchiveBar {
    GtkInfoBar parent;
    NemoArchiveBarPrivate *priv;
};

struct _NemoArchiveBarClass {
    GtkInfoBarClass parent_class;
};

GType      nemo_archive_bar_get_type (void);
GtkWidget *nemo_archive_bar_new      (NemoView *view);

#endif /* NEMO_ARCHIVE_BAR_H */
