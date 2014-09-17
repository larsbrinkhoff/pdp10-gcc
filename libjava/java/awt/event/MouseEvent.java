/* Copyright (C) 2000, 2002  Free Software Foundation

   This file is part of libjava.

This software is copyrighted work licensed under the terms of the
Libjava License.  Please consult the file "LIBJAVA_LICENSE" for
details.  */

package java.awt.event;
import java.awt.*;

/**
 * @author Tom Tromey <tromey@cygnus.com>
 * @date April 8, 2000
 */

/* Status: Believed complete and correct to JDK 1.2.  */

public class MouseEvent extends InputEvent
{
  public static final int MOUSE_CLICKED = 500;
  public static final int MOUSE_DRAGGED = 506;
  public static final int MOUSE_ENTERED = 504;
  public static final int MOUSE_EXITED = 505;
  public static final int MOUSE_FIRST = 500;
  public static final int MOUSE_LAST = 506;
  public static final int MOUSE_MOVED = 503;
  public static final int MOUSE_PRESSED = 501;
  public static final int MOUSE_RELEASED = 502;

  public MouseEvent (Component source, int id, long when, int modifiers,
		     int x, int y, int clickCount, boolean popupTrigger)
  {
    super (source, id);
    this.when = when;
    this.modifiers = modifiers;
    this.x = x;
    this.y = y;
    this.clickCount = clickCount;
    this.popupTrigger = popupTrigger;
  }

  public int getClickCount ()
  {
    return clickCount;
  }

  public Point getPoint ()
  {
    return new Point (x, y);
  }

  public int getX ()
  {
    return x;
  }

  public int getY ()
  {
    return y;
  }

  public boolean isPopupTrigger ()
  {
    return popupTrigger;
  }

  public String paramString ()
  {
    String r;
    switch (id)
      {
	case MOUSE_CLICKED:
	  r = "MOUSE_CLICKED";
	break;
	case MOUSE_DRAGGED:
	  r = "MOUSE_DRAGGED";
	break;
	case MOUSE_ENTERED:
	  r = "MOUSE_ENTERED";
	break;
	case MOUSE_EXITED:
	  r = "MOUSE_EXITED";
	break;
	case MOUSE_MOVED:
	  r = "MOUSE_MOVED";
	break;
	case MOUSE_PRESSED:
	  r = "MOUSE_PRESSED";
	break;
	case MOUSE_RELEASED:
	  r = "MOUSE_RELEASED";
	break;
	default:
	  r = "unknown id";
	break;
      }
    r += ",(" + x + "," + y + "),modifiers=" + modifiers + ",clickCount=" +
         clickCount;
    return r;
  }

  public void translatePoint (int x, int y)
  {
    this.x += x;
    this.y += y;
  }

  private int x;
  private int y;
  private int clickCount;
  private boolean popupTrigger;
}
