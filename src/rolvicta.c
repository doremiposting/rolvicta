#include <gtk/gtk.h>
#include <math.h>

#define DISC_LABEL_RATIO 0.40
#define DISC_HOLE_RATIO 0.02

#define METADATA_MARGIN 24.0
#define SONG_FONT_SIZE 20.0
#define ALBUM_FONT_SIZE 24.0
#define ARTIST_FONT_SIZE 48.0
#define METADATA_BOX_PADDING 8.0
#define METADATA_GAP 8.0
#define SHADOW_OFFSET 2.0

#define COLOR_HOTPINK_R 1.0000
#define COLOR_HOTPINK_G 0.4118
#define COLOR_HOTPINK_B 0.7059

static void
rendergroovetex(cairo_t *cr, double cx, double cy, double radius) {
  double r, shade;
  cairo_save(cr);

  cairo_set_source_rgb(cr, 0.05, 0.05, 0.05);
  cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_set_line_width(cr, 1.0);
  for (r = radius * 0.45; r < radius; r += 2.0) {
    shade = 0.08 + 0.04 * sin(r * 0.7);
    cairo_set_source_rgba(cr, 1, 1, 1, shade);
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
    cairo_stroke(cr);
  }
  
  cairo_restore(cr);
}

static cairo_surface_t *
buildgroovecache(double rad) {
  cairo_surface_t *s;
  cairo_t *cr;
  int dim;
  dim = (int)ceil(rad * 2.0);
  s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dim, dim);
  cr = cairo_create(s);

  rendergroovetex(cr, rad, rad, rad);

  cairo_destroy(cr);
  return s;
}

static void
renderalbummd(cairo_t *cr, int height,
              const char *songname, const char *albumname) {
  cairo_text_extents_t ext;
  cairo_font_extents_t fext;
  double boxw, boxh, boxx, boxy, basex, basey;

  cairo_save(cr);
  /* TODO: have "sans" be configurable */
  cairo_select_font_face(cr, "Serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, ALBUM_FONT_SIZE);
  cairo_font_extents(cr, &fext);

  boxx = METADATA_MARGIN;
  boxy = height - METADATA_MARGIN;

  if (albumname) {
    cairo_text_extents(cr, albumname, &ext);
    boxw = ext.width + 2 * METADATA_BOX_PADDING;
    boxh = fext.ascent + fext.descent + 2 * METADATA_BOX_PADDING;
    boxy = height - METADATA_MARGIN - boxh;

    cairo_set_source_rgb(cr,
        COLOR_HOTPINK_R, COLOR_HOTPINK_G, COLOR_HOTPINK_B);
    cairo_rectangle(cr, boxx, boxy, boxw, boxh);
    cairo_fill(cr);

    basex = boxx + METADATA_BOX_PADDING - ext.x_bearing;
    basey = boxy + METADATA_BOX_PADDING + fext.ascent;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, basex, basey);
    cairo_show_text(cr, albumname);
  }
  
  /* TODO: have "sans" be configurable */
  cairo_select_font_face(cr, "Serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, SONG_FONT_SIZE);
  cairo_font_extents(cr, &fext);

  if (songname) {
    cairo_text_extents(cr, songname, &ext);
    basex = METADATA_MARGIN - ext.x_bearing;
    basey = boxy - METADATA_GAP;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, basex, basey);
    cairo_show_text(cr, songname);
  }

  cairo_restore(cr);
}

static void
renderartistname(cairo_t *cr, int width, int height,
                const char *artistname) {
  if (!artistname) { return; }

  cairo_save(cr);
  /* TODO: have "sans" be configurable */
  cairo_select_font_face(cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, ARTIST_FONT_SIZE);

  cairo_translate(cr, width - METADATA_MARGIN, height - METADATA_MARGIN);
  cairo_rotate(cr, -M_PI / 2.0);

  /* The fake drop shadow */
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, SHADOW_OFFSET, SHADOW_OFFSET);
  cairo_show_text(cr, artistname);

  /* The real test */
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_move_to(cr, 0, 0);
  cairo_show_text(cr, artistname);

  cairo_restore(cr);
}


static void
renderlabel(cairo_t *cr, cairo_surface_t *albumart,
            double cx, double cy, double radius) {
  int aw, ah;
  double scale;
  cairo_save(cr);

  cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
  cairo_clip(cr);

  if (albumart) {
    aw = cairo_image_surface_get_width(albumart);
    ah = cairo_image_surface_get_height(albumart);
    scale = (2 * radius) / (double)(aw < ah ? aw : ah);
    cairo_translate(cr, cx - radius, cy - radius);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, albumart, 0, 0);
    cairo_paint(cr);
  } else {
    /* TODO: Change this to a different image instead of a solid color. */
    /* TODO: Make that different image configurable with a define above. */
    cairo_set_source_rgb(cr, 0.75, 0.1, 0.15);
    cairo_paint(cr);
  }

  cairo_restore(cr);
}

static void
renderhole(cairo_t *cr, double cx, double cy, double radius) {
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
  cairo_fill(cr);
  cairo_restore(cr);
}

static void
renderdisc(cairo_t *cr, int width, int height,
          double anglerad, cairo_surface_t *albumart,
          cairo_surface_t **groovecache, double *groovecacherad) {
  double cx, cy, discrad, mindim;
  mindim = width < height ? width : height;

  discrad = mindim / DISC_LABEL_RATIO / 2.0 * 0.80;

  if (width >= height) {
    /* Landscape, or treat as landscape */
    cx = width * (5.0 / 6.0);
    cy = height / 2.0;
  } else {
    /* Portrait, or close enough */
    cx = width / 2.0;
    cy = height / 6.0;
  }

  cairo_save(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);

  cairo_translate(cr, cx, cy);
  cairo_rotate(cr, anglerad);
  cairo_translate(cr, -cx, -cy);

  if (!*groovecache || *groovecacherad != discrad) {
    if (*groovecache) { cairo_surface_destroy(*groovecache); }
    *groovecache = buildgroovecache(discrad);
    *groovecacherad = discrad;
  }
  cairo_save(cr);
  cairo_translate(cr, cx-discrad, cy-discrad);
  cairo_set_source_surface(cr, *groovecache, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  renderlabel(cr, albumart, cx, cy, discrad * DISC_LABEL_RATIO);
  renderhole(cr, cx, cy, discrad * DISC_HOLE_RATIO);

  cairo_restore(cr);
}

typedef struct {
  double angle;
  gboolean playing;
  cairo_surface_t *albumart;
  char *songname;
  char *albumname;
  char *artistname;
} Appstate;

static void
appstateinit(Appstate *st) {
  st->angle = 0.0;
  st->playing = FALSE;
  st->albumart = NULL;
  st->songname = NULL;
  st->albumname = NULL;
  st->artistname = NULL;
}

static void
appstatetick(Appstate *st, double dtsecs) {
  const double RADIANS_PER_SEC = 
    2 * M_PI * (4.5 / 60.0);
  if (st->playing) {
    st->angle = fmod(st->angle + RADIANS_PER_SEC * dtsecs, 2 * M_PI);
  }
}

typedef struct {
  GtkWidget *drawarea;
  Appstate state;
  guint timerid;
  gint64 prevtickus;
  cairo_surface_t *groovecache;
  double groovecacherad;
} Gtkapp;

static gboolean
gtkondraw(GtkWidget *widget, cairo_t *cr, gpointer userdata) {
  Gtkapp *app;
  int width, height;
  app = userdata;
  width = gtk_widget_get_allocated_width(widget);
  height = gtk_widget_get_allocated_height(widget);

  renderdisc(cr,width, height, app->state.angle, app->state.albumart,
      &app->groovecache, &app->groovecacherad);
  renderalbummd(cr, height, app->state.songname, app->state.albumname);
  renderartistname(cr, width, height, app->state.artistname);

  return FALSE;
}

static gboolean
gtkonframeclock(GtkWidget *widget, GdkFrameClock *clock,
                gpointer userdata) {
  Gtkapp *app;
  gint64 rn;
  double dt;
  app = userdata;
  rn = g_get_monotonic_time();
  dt = (rn - app->prevtickus) / 1e6;
  app->prevtickus = rn;
  
  appstatetick(&app->state, dt);
  gtk_widget_queue_draw(app->drawarea);

  return G_SOURCE_CONTINUE;
}

static void
gtkappsetplaying(Gtkapp *app, gboolean playing) {
  app->state.playing = playing;
}

static void
gtkappsetalbumart(Gtkapp *app, const char *fp /* png file */) {
  GdkPixbuf *pixbuf;
  GError *err;
  if (app->state.albumart) { cairo_surface_destroy(app->state.albumart); }
  app->state.albumart = NULL;
  if (fp) {
    err = NULL;
    pixbuf = gdk_pixbuf_new_from_file(fp, &err);
    if (pixbuf) {
      app->state.albumart = gdk_cairo_surface_create_from_pixbuf(pixbuf, 1, NULL);
      g_object_unref(pixbuf);
    } else {
      g_warning("Failed to load album art '%s': %s", fp, err->message);
      g_error_free(err);
    }
  }
  if (app->state.albumart &&
      cairo_surface_status(app->state.albumart) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(app->state.albumart);
    app->state.albumart = NULL;
  }
}

static void
gtkappsetsongname(Gtkapp *app, const char *s) {
  /* TODO: At risk of double-free? */
  g_free(app->state.songname);
  app->state.songname = s ? g_strdup(s) : NULL;
}

static void
gtkappsetalbumname(Gtkapp *app, const char *s) {
  /* TODO: At risk of double-free? */
  g_free(app->state.albumname);
  app->state.albumname = s ? g_strdup(s) : NULL;
}

static void
gtkappsetartistname(Gtkapp *app, const char *s) {
  /* TODO: At risk of double-free? */
  g_free(app->state.artistname);
  app->state.artistname = s ? g_strdup(s) : NULL;
}

static void
gtkondestroy(GtkWidget *widget, gpointer userdata) {
  Gtkapp *app;
  app = userdata;
  if (app->timerid) {
    gtk_widget_remove_tick_callback(app->drawarea, app->timerid);
  }
  if (app->groovecache) { cairo_surface_destroy(app->groovecache); }
  if (app->state.albumart) {
    cairo_surface_destroy(app->state.albumart);
  }
  /* TODO: At risk of double-free? */
  g_free(app->state.songname);
  g_free(app->state.albumname);
  g_free(app->state.artistname);
  gtk_main_quit();
}

int
main(int argc, char *argv[]) {
  GtkWidget *window;
  Gtkapp app;

  gtk_init(&argc, &argv);
  appstateinit(&app.state);
  app.prevtickus = g_get_monotonic_time();
  app.groovecache = NULL;
  app.groovecacherad = -1.0;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "rolvicta");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);

  app.drawarea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window), app.drawarea);

  g_signal_connect(app.drawarea, "draw", G_CALLBACK(gtkondraw), &app);
  g_signal_connect(window, "destroy", G_CALLBACK(gtkondestroy), &app);
  app.timerid = gtk_widget_add_tick_callback(
      app.drawarea, gtkonframeclock, &app, NULL);

/* #if 0 */
  /* XXX: Temporary for testing. */
  if (argc > 1) {
    gtkappsetalbumart(&app, argv[1]);
  }
  if (argc > 2) { gtkappsetsongname(&app, argv[2]); }
  if (argc > 3) { gtkappsetalbumname(&app, argv[3]); }
  if (argc > 4) { gtkappsetartistname(&app, argv[4]); }
  /* XXX: Temporary for testing. */
/* #endif */

  gtkappsetplaying(&app, TRUE);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}
