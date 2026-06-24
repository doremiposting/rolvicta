#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <mpd/client.h>

#define DISC_LABEL_RATIO 0.40
#define DISC_HOLE_RATIO 0.02

#define METADATA_MARGIN 24.0
#define SONG_FONT_SIZE 20.0
#define ALBUM_FONT_SIZE 24.0
#define ARTIST_FONT_SIZE 48.0
#define METADATA_BOX_PADDING 8.0
#define METADATA_GAP 8.0
#define SHADOW_OFFSET 2.0
#define PLAYLIST_FONT_SIZE 36.0
#define PLAYLIST_LINE_HEIGHT (1.25 * PLAYLIST_FONT_SIZE)
/* TODO: Change this to a global and calculate based on...
 * METADATA MARGIN + album box + song gap+font + padding */
#define PLAYLIST_BOTTOM_RESERVE 100.0
#define STATUS_FONT_SIZE 16.0
#define STATUS_BOX_PADDING 6.0
#define STATUS_ROW_HEIGHT (STATUS_FONT_SIZE + 2.0 * STATUS_BOX_PADDING + 4.0)
#define TONEARM_MARGIN 30.0
#define TONEARM_LS_THETA_START (160.0 * M_PI / 180.0)
#define TONEARM_LS_THETA_END (200.0 * M_PI / 180.0)
#define TONEARM_PT_THETA_START (70.0 * M_PI / 180.0)
#define TONEARM_PT_THETA_END (110.0 * M_PI / 180.0)

#define COLOR_HOTPINK_R 1.0000
#define COLOR_HOTPINK_G 0.4118
#define COLOR_HOTPINK_B 0.7059

#define COLOR_CREAM_R 1.0000
#define COLOR_CREAM_G 0.9600
#define COLOR_CREAM_B 0.7000

double recordcx, recordcy, discrad;
int isportrait;

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
              const char *songname, const char *albumname,
              int elapsed, int duration, const char *modestr) {
  cairo_text_extents_t ext;
  cairo_text_extents_t mext;
  cairo_font_extents_t fext;
  double boxw, boxh, boxx, boxy, basex, basey;
  double modeboxx, modeboxw, modeboxh, modebasey;
  /* TODO: No way this buffer needs to be so long. */
  /* TODO: Beyond some amount of time, replace that time with "Lots!" */
  char songtimebuf[512];
  int elapsedm, elapseds, durationm, durations;

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
    elapsedm = elapsed / 60;
    elapseds = elapsed % 60;
    durationm = duration / 60;
    durations = duration % 60;
    if (duration > 0) {
      snprintf(songtimebuf, sizeof(songtimebuf), "%s (%d:%02d/%d:%02d)",
          songname, elapsedm, elapseds, durationm, durations);
    } else {
      snprintf(songtimebuf, sizeof(songtimebuf), "%s", songname);
    }
    cairo_text_extents(cr, songtimebuf, &ext);
    basex = METADATA_MARGIN - ext.x_bearing;
    basey = boxy - METADATA_GAP;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, basex, basey);
    cairo_show_text(cr, songtimebuf);

    if (modestr) {
      modeboxx = basex + ext.x_advance + METADATA_GAP;
      cairo_text_extents(cr, modestr, &mext);
      modeboxw = mext.width + 2.0 * METADATA_BOX_PADDING;
      modeboxh = fext.ascent + fext.descent + 2.0 * METADATA_BOX_PADDING;
      modebasey = basey - fext.ascent - METADATA_BOX_PADDING;
      cairo_set_source_rgb(cr,
          COLOR_HOTPINK_R, COLOR_HOTPINK_G, COLOR_HOTPINK_B);
      cairo_rectangle(cr, modeboxx, modebasey, modeboxw, modeboxh);
      cairo_fill(cr);
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
      cairo_move_to(cr, modeboxx + METADATA_BOX_PADDING - mext.x_bearing, basey);
      cairo_show_text(cr, modestr);
    }
  }

  cairo_restore(cr);
}

static void
rendertonearm(cairo_t *cr, int width, int height,
    double cx, double cy, double discrad, double progress) {
  double angle, sx, sy, ax, ay;
  double stylusst, stylusend, theta;
  double router, rinner, contact;

  if (progress < 0.0) { progress = 0.0; }
  if (progress > 1.0) { progress = 1.0; }

  if (width >= height) {
    theta = TONEARM_LS_THETA_START
      + progress 
      * (TONEARM_LS_THETA_END - TONEARM_LS_THETA_START);
  } else {
    theta = TONEARM_PT_THETA_START
      + progress 
      * (TONEARM_PT_THETA_END - TONEARM_PT_THETA_START);
  }
  router = discrad;
  rinner = discrad * 0.45;
  contact = router + progress * (rinner - router);
  sx = cx + cos(theta) * contact;
  sy = cy + sin(theta) * contact;

  cairo_save(cr);
  /* arm */
  cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
  cairo_set_line_width(cr, 12.0);
  if (width >= height) {
    ax = width - (discrad * 0.75);
    ay = height;
    cairo_move_to(cr, ax, ay);
    cairo_curve_to(cr,
        ax, ay + 200,
        sx - 100, sy - 100,
        sx, sy);
  } else {
    ax = width;
    ay = discrad * 0.75;
    cairo_move_to(cr, ax, ay);
    cairo_curve_to(cr,
        ax, ay - 150,
        sx - 150, sy,
        sx, sy);
  }
  cairo_stroke(cr);
  /* stylus */
  cairo_arc(cr, sx, sy, 8.0, 0, 2 * M_PI);
  cairo_fill(cr);
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
  double scale, srcx, srcy;
  cairo_save(cr);

  srcx = srcy = 0.0;

  cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
  cairo_clip(cr);

  if (albumart) {
    aw = cairo_image_surface_get_width(albumart);
    ah = cairo_image_surface_get_height(albumart);
    if (aw > ah) { srcx = (aw - ah) / 2.0; }
    else if (ah > aw) { srcy = (ah - aw) / 2.0; }
    scale = (2 * radius) / (double)(aw < ah ? aw : ah);
    cairo_translate(cr, cx - radius, cy - radius);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, albumart, -srcx, -srcy);
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
renderplaylist(cairo_t *cr, int height, enum mpd_state state,
    char **tracks, int count, int current) {
  cairo_text_extents_t ext;
  cairo_font_extents_t fext;
  int maxvis, start, end, i;
  double boxx, boxy, boxw, boxh, basex, basey, nextboxx, top, pltop;
  const char *statestr;

  cairo_save(cr);

  if (isportrait) {
    top = recordcy + discrad * DISC_LABEL_RATIO + 20.0;
  } else {
    top = METADATA_MARGIN;
  }

  /* XXX: Maxvis was being calculated here, but moved down...
   * If we ever encounter bugs with the status boxes being rendered
   * outside where they should, it's because this got moved. */

  cairo_select_font_face(cr, "Serif",
      CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, PLAYLIST_FONT_SIZE);
  cairo_font_extents(cr, &fext);

  boxh = fext.ascent + fext.descent + 2.0 * STATUS_BOX_PADDING;
  boxy = top;
  basey = boxy + STATUS_BOX_PADDING + fext.ascent;
  boxx = METADATA_MARGIN;

  /* TODO: When the time comes to implement multiple views,
   * it's this block below that needs to change. */
  cairo_text_extents(cr, "Playlist View", &ext);
  boxw = ext.width + 2.0 * STATUS_BOX_PADDING;
  cairo_set_source_rgb(cr,
      COLOR_HOTPINK_R, COLOR_HOTPINK_G, COLOR_HOTPINK_B);
  cairo_rectangle(cr, boxx, boxy, boxw, boxh);
  cairo_fill(cr);
  basex = boxx + STATUS_BOX_PADDING - ext.x_bearing;
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, basex, basey);
  cairo_show_text(cr, "Playlist View");

  nextboxx = boxx + boxw + METADATA_GAP;

  switch (state) {
    case MPD_STATE_PLAY:
      statestr = "Playing";
      cairo_set_source_rgb(cr, 0.18, 0.65, 0.18);
      break;
    case MPD_STATE_PAUSE:
      statestr = "Paused";
      cairo_set_source_rgb(cr, 0.75, 0.65, 0.10);
      break;
    case MPD_STATE_STOP:
      statestr = "Stopped";
      cairo_set_source_rgb(cr, 0.75, 0.18, 0.18);
      break;
    default:
      statestr = "Unknown";
      cairo_set_source_rgb(cr, 0.50, 0.10, 0.75);
      break;
  }
  cairo_text_extents(cr, statestr, &ext);
  boxw = ext.width + 2.0 * STATUS_BOX_PADDING;
  cairo_rectangle(cr, nextboxx, boxy, boxw, boxh);
  cairo_fill(cr);
  basex = nextboxx + STATUS_BOX_PADDING - ext.x_bearing;
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, basex, basey);
  cairo_show_text(cr, statestr);

  if (!tracks || count == 0) { cairo_restore(cr); return; }
  
  pltop = top + STATUS_ROW_HEIGHT + METADATA_GAP;
  maxvis = (int)((height -  PLAYLIST_BOTTOM_RESERVE - pltop)
              / PLAYLIST_LINE_HEIGHT);
  if (maxvis <= 0) { cairo_restore(cr); return; }

  if (current < 0) { start = 0; }
  else {
    start = current - maxvis / 2;
    if (start + maxvis > count) { start = count - maxvis; }
    if (start < 0) { start = 0; }
  }
  end = start + maxvis;
  if (end > count) { end = count; }

  for (i = start; i < end; i++) {
    basey = top + boxh 
            + (double)(i - start) * PLAYLIST_LINE_HEIGHT
            + fext.ascent;
    if (i == current) {
      cairo_set_source_rgb(cr, COLOR_CREAM_R, COLOR_CREAM_G, COLOR_CREAM_B);
    } else {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    }
    cairo_move_to(cr, METADATA_MARGIN, basey);
        cairo_show_text(cr, tracks[i]);
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

/* TODO:
 * [16:11]real snowpity that you can affor: @everyone (me) (banned x 2) are you a bad enough dude to allow it to be used for seeking?
 * [16:11]real snowpity that you can affor: bonus points for somehow putting record scratching in
 * This belongs somewhere near event handling, but:
 * Click on record, seek to that approx. position. Click & drag, pause until
 * mouse releases, extra: play record scratch on click and release */
static void
renderdisc(cairo_t *cr, int width, int height,
          double anglerad, cairo_surface_t *albumart,
          cairo_surface_t **groovecache, double *groovecacherad) {
  double mindim;
  mindim = width < height ? width : height;

  discrad = mindim / DISC_LABEL_RATIO / 2.0 * 0.80;

  if (width >= height) {
    /* Landscape, or treat as landscape */
    isportrait = 0;
    recordcx = width * (5.0 / 6.0);
    recordcy = height / 2.0;
  } else {
    /* Portrait, or close enough */
    isportrait = 1;
    recordcx = width / 2.0;
    recordcy = height / 6.0;
  }

  cairo_save(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);

  cairo_translate(cr, recordcx, recordcy);
  cairo_rotate(cr, anglerad);
  cairo_translate(cr, -recordcx, -recordcy);

  if (!*groovecache || *groovecacherad != discrad) {
    if (*groovecache) { cairo_surface_destroy(*groovecache); }
    *groovecache = buildgroovecache(discrad);
    *groovecacherad = discrad;
  }
  cairo_save(cr);
  cairo_translate(cr, recordcx-discrad, recordcy-discrad);
  cairo_set_source_surface(cr, *groovecache, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  renderlabel(cr, albumart, recordcx, recordcy, discrad * DISC_LABEL_RATIO);
  renderhole(cr, recordcx, recordcy, discrad * DISC_HOLE_RATIO);

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
  struct mpd_connection *mpdc;
  guint mpdtimerid;
  char *mpdsonguri;
  int mpdelapsed;
  int mpdduration;
  char modestr[9];
  enum mpd_state mpdstate;
  char **playlist;
  int plcount;
  int plpos;
} Gtkapp;

static gboolean
gtkondraw(GtkWidget *widget, cairo_t *cr, gpointer userdata) {
  Gtkapp *app;
  int width, height;
  double mindim, cx, cy, discrad, progress;
  app = userdata;
  width = gtk_widget_get_allocated_width(widget);
  height = gtk_widget_get_allocated_height(widget);

  renderdisc(cr,width, height, app->state.angle, app->state.albumart,
      &app->groovecache, &app->groovecacherad);
  mindim = (double)(width < height ? width : height);
  discrad = mindim / DISC_LABEL_RATIO / 2.0 * 0.80;
  if (width >= height) {
    cx = (double)width * (5.0 / 6.0);
    cy = (double)height / 2.0;
  } else {
    cx = (double)width / 2.0;
    cy = (double)height / 6.0;
  }
  if (app->mpdduration > 0) {
    progress = (double)app->mpdelapsed / (double)app->mpdduration;
    if (progress > 1.0) { progress = 1.0; }
    rendertonearm(cr, width, height, cx, cy, discrad, progress);
  }
  renderplaylist(cr, height, app->mpdstate,
      app->playlist, app->plcount, app->plpos);
  renderalbummd(cr, height, app->state.songname, app->state.albumname,
      app->mpdelapsed, app->mpdduration, app->modestr);
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
  GdkPixbuf *pb;
  GError *err;
  if (app->state.albumart) { cairo_surface_destroy(app->state.albumart); }
  app->state.albumart = NULL;
  if (fp) {
    err = NULL;
    pb = gdk_pixbuf_new_from_file(fp, &err);
    if (pb) {
      app->state.albumart = gdk_cairo_surface_create_from_pixbuf(pb, 1, NULL);
      g_object_unref(pb);
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
gtkappsetplaylist(Gtkapp *app, char **tracks, int count) {
  int i;
  if (app->playlist) {
    for (i = 0; i < app->plcount; i++) {
      g_free(app->playlist[i]);
    }
    g_free(app->playlist);
  }
  app->playlist = tracks;
  app->plcount = count;
}

static gboolean
mpdconnect(Gtkapp *app) {
  enum mpd_error err;

  app->mpdc = mpd_connection_new(NULL, 0, 0);
  if (!app->mpdc) { return FALSE; }
  err = mpd_connection_get_error(app->mpdc);
  if (err != MPD_ERROR_SUCCESS) {
    g_warning("mpd connection failed: %s",
        mpd_connection_get_error_message(app->mpdc));
    mpd_connection_free(app->mpdc);
    app->mpdc = NULL;
    return FALSE;
  }

  return TRUE;
}

static void
gtkappsetalbumartbuf(Gtkapp *app, const guint8 *data, gsize len) {
  GdkPixbufLoader *ldr;
  GdkPixbuf *pb;
  GError *err;

  if (app->state.albumart) { cairo_surface_destroy(app->state.albumart); }
  app->state.albumart = NULL;

  if (!data || len == 0) { return; }

  err = NULL;
  ldr = gdk_pixbuf_loader_new();
  if (!gdk_pixbuf_loader_write(ldr, data, len, &err)) {
    g_warning("Failed to decode album art: %s", err->message);
    g_error_free(err);
    g_object_unref(ldr);
    return;
  }
  if (!gdk_pixbuf_loader_close(ldr, &err)) {
    g_warning("Started decoding album art but never finished: %s",
        err->message);
    g_error_free(err);
    g_object_unref(ldr);
    return;
  }

  pb = gdk_pixbuf_loader_get_pixbuf(ldr);
  if (pb) {
    app->state.albumart = gdk_cairo_surface_create_from_pixbuf(
        pb, 1, NULL
        );
  }
  g_object_unref(ldr);

  if (app->state.albumart &&
      cairo_surface_status(app->state.albumart) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(app->state.albumart);
    app->state.albumart = NULL;
  }
}

#define MPD_ART_CHUNK 8192
typedef int (*mpdartfetchfn)(struct mpd_connection *connection,
                            const char *uri, unsigned offset,
                            void *buffer, size_t buffersz);

static void
mpdreadart(Gtkapp *app, const char *uri, mpdartfetchfn ffn,
    guint8 **outart, gsize *outartlen) {
  guint8 *art;
  gsize artlen, artcap;
  guint8 chunk[MPD_ART_CHUNK];
  unsigned offset;
  int n;

  art = NULL;
  artlen = 0;
  artcap = 0;
  offset = 0;

  for (;;) {
    n = ffn(app->mpdc, uri, offset, chunk, sizeof(chunk));
    if (n < 0) { break; }

    if (artlen + (gsize)n > artcap) {
      artcap = artlen + (gsize)n;
      art = g_realloc(art, artcap);
    }
    memcpy(art + artlen, chunk, (size_t)n);
    artlen += (gsize)n;
    offset += (unsigned)n;

    if ((size_t)n < sizeof(chunk)) { break; }
  }
  *outart = art;
  *outartlen = artlen;
}

static void
mpdfetchalbumart(Gtkapp *app, const char *uri) {
  guint8 *art;
  gsize artlen;
  
  mpdreadart(app, uri, mpd_run_readpicture, &art, &artlen);
  if (artlen == 0) {
    g_free(art);
    /* XXX: mpd_run_albumart will block GTK on slow connections */
    mpdreadart(app, uri, mpd_run_albumart, &art, &artlen);
  }

  gtkappsetalbumartbuf(app, art, artlen);
  g_free(art);
}

static void
mpdfetchplaylist(Gtkapp *app) {
  struct mpd_song *song;
  const char *title;
  char **tracks;
  int count, cap;

  tracks = NULL;
  count = 0;
  cap = 0;

  if (!mpd_send_list_queue_meta(app->mpdc)) { return; }

  while ((song = mpd_recv_song(app->mpdc)) != NULL) {
    if (count >= cap) {
      cap = cap ? cap * 2 : 16;
      tracks = g_realloc(tracks, (gsize)cap * sizeof(char*));
    }
    title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    tracks[count++] = g_strdup(title ? : "/shrug");
    mpd_song_free(song);
  }
  mpd_response_finish(app->mpdc);

  gtkappsetplaylist(app, tracks, count);
}

static gboolean
mpdpoll(gpointer userdata) {
  Gtkapp *app;
  struct mpd_status *status;
  struct mpd_song *song;
  enum mpd_state state;
  int songpos;
  int elapsed, total;
  const char *songuri, *title, *album, *artist;

  app = userdata;

  if (!app->mpdc && !mpdconnect(app)) {
    return G_SOURCE_CONTINUE;
  }

  status = mpd_run_status(app->mpdc);
  if (!status) {
    mpd_connection_free(app->mpdc);
    app->mpdc = NULL;
    return G_SOURCE_CONTINUE;
  }
  state = mpd_status_get_state(status);
  elapsed = (int)mpd_status_get_elapsed_time(status);
  total = (int)mpd_status_get_total_time(status);
  app->mpdstate = state;
  app->mpdelapsed = elapsed;
  app->mpdstate = state;
  app->mpdduration = total;
  app->modestr[1] = mpd_status_get_repeat(status)  ? 'r' : '-';
  app->modestr[2] = mpd_status_get_random(status)  ? 'z' : '-';
  app->modestr[3] = mpd_status_get_single(status)  ? 's' : '-';
  app->modestr[4] = mpd_status_get_consume(status)  ? 'c' : '-';
  app->modestr[5] = mpd_status_get_crossfade(status) > 0  ? 'x' : '-';
  app->modestr[6] = mpd_status_get_update_id(status) > 0  ? 'U' : '-';
  songpos = mpd_status_get_song_pos(status);
  gtkappsetplaying(app, state == MPD_STATE_PLAY);
  mpd_status_free(status);
  app->plpos = songpos;

  song = mpd_run_current_song(app->mpdc);
  if (!song) {
    gtkappsetsongname(app, NULL);
    gtkappsetalbumname(app, NULL);
    gtkappsetartistname(app, NULL);
    gtkappsetalbumartbuf(app, NULL, 0);
    g_free(app->mpdsonguri);
    app->mpdsonguri = NULL;
    return G_SOURCE_CONTINUE;
  }

  songuri = mpd_song_get_uri(song);
  if (!app->mpdsonguri || strcmp(app->mpdsonguri, songuri) != 0) {
    title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);

    gtkappsetsongname(app, title);
    gtkappsetalbumname(app, album);
    gtkappsetartistname(app, artist);

    mpdfetchalbumart(app, songuri);
    mpdfetchplaylist(app);

    g_free(app->mpdsonguri);
    app->mpdsonguri = g_strdup(songuri);
  }

  mpd_song_free(song);

  return G_SOURCE_CONTINUE;
}

static void
gtkondestroy(GtkWidget *widget, gpointer userdata) {
  Gtkapp *app;
  app = userdata;
  if (app->timerid) {
    gtk_widget_remove_tick_callback(app->drawarea, app->timerid);
  }
  if (app->mpdtimerid) { g_source_remove(app->mpdtimerid); }
  if (app->mpdc) { mpd_connection_free(app->mpdc); }
  g_free(app->mpdsonguri);
  gtkappsetplaylist(app, NULL, 0);
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

static gboolean
gtkonkeypress(GtkWidget *widget, GdkEventKey *ev, gpointer userdata) {
  Gtkapp *app;
  app = userdata;
  (void)widget;
  if (!app->mpdc) { return FALSE; }

  switch (ev->keyval) {
    case GDK_KEY_greater:
      mpd_run_next(app->mpdc);
      break;
    case GDK_KEY_less:
      mpd_run_previous(app->mpdc);
      break;
    case GDK_KEY_p:
      mpd_run_toggle_pause(app->mpdc);
      break;
    case GDK_KEY_s:
      mpd_run_stop(app->mpdc);
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
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
  app.mpdc = NULL;
  app.mpdsonguri = NULL;
  app.playlist = NULL;
  app.plcount = 0;
  app.plpos = -1;
  app.mpdelapsed = 0;
  app.mpdduration = 0;
  app.mpdstate = MPD_STATE_UNKNOWN;
  memcpy(app.modestr, "[------]", 9);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "rolvicta");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);

  app.drawarea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window), app.drawarea);

  g_signal_connect(app.drawarea, "draw", G_CALLBACK(gtkondraw), &app);
  g_signal_connect(window, "destroy", G_CALLBACK(gtkondestroy), &app);
  g_signal_connect(window, "key-press-event", G_CALLBACK(gtkonkeypress), &app);
  app.timerid = gtk_widget_add_tick_callback(
      app.drawarea, gtkonframeclock, &app, NULL);
  app.mpdtimerid = g_timeout_add(1000, mpdpoll, &app);

  (void)argc; (void)argv;
  gtkappsetplaying(&app, TRUE);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}
