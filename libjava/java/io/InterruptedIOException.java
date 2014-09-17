/* InterruptedIOException.java -- An I/O operation was interrupted.
   Copyright (C) 1998, 1999, 2001 Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.
 
GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


package java.io;

/* Written using "Java Class Libraries", 2nd edition, ISBN 0-201-31002-3
 * "The Java Language Specification", ISBN 0-201-63451-1
 * Status:  Complete to 1.1.
 */

/**
  * This exception is thrown when a in process I/O operation is 
  * interrupted for some reason.  The field bytesTransferred will contain
  * the number of bytes that were read/written prior to the interruption.
  *
  * @version 0.0
  *
  * @author Aaron M. Renn (arenn@urbanophile.com)
  * @author Tom Tromey <tromey@cygnus.com>
  * @date September 24, 1998
  */
public class InterruptedIOException extends IOException
{

private static final long serialVersionUID = 4020568460727500567L;

/*
 * Instance Variables
 */

/**
  * The number of bytes read/written prior to the interruption
  */
public int bytesTransferred;

/*************************************************************************/

/*
 * Constructors
 */

/**
  * Create a new InterruptedIOException without a descriptive error message
  */
public
InterruptedIOException()
{
  super();
}

/*************************************************************************/

/**
  * Create a new InterruptedIOException with a descriptive error message String
  *
  * @param message The descriptive error message
  */
public
InterruptedIOException(String message)
{
  super(message);
}

/*************************************************************************/

/**
  * Create a new InterruptedIOException with a descriptive error message 
  * String.  Also sets the value of the bytesTransferred field.
  * 
  * @param message The descriptive error message
  * @param bytesTransferred The number of bytes tranferred before the interruption
  */
InterruptedIOException(String message, int bytesTransferred)
{
  super(message);
  this.bytesTransferred = bytesTransferred;
}

} // class InterruptedIOException

