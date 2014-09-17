// ClassLoader.java - Define policies for loading Java classes.

/* Copyright (C) 1998, 1999, 2000, 2001  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

package java.lang;

import java.io.InputStream;
import java.io.IOException;
import java.net.URL;
import java.net.URLConnection;
import java.security.AllPermission;
import java.security.CodeSource;
import java.security.Permission;
import java.security.Permissions;
import java.security.Policy;
import java.security.ProtectionDomain;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Stack;

/**
 * The class <code>ClassLoader</code> is intended to be subclassed by
 * applications in order to describe new ways of loading classes,
 * such as over the network.
 *
 * @author  Kresten Krab Thorup
 */

public abstract class ClassLoader
{
  private ClassLoader parent;
  private HashMap definedPackages = new HashMap();

  public final ClassLoader getParent ()
  {
    /* FIXME: security */
    return parent;
  }

  public static ClassLoader getSystemClassLoader ()
  {
    return gnu.gcj.runtime.VMClassLoader.instance;
  }

  /**
   * Creates a <code>ClassLoader</code> with no parent.
   * @exception java.lang.SecurityException if not allowed
   */
  protected ClassLoader() 
  {
    this (null);
  }

  /**
   * Creates a <code>ClassLoader</code> with the given parent.   
   * The parent may be <code>null</code>.
   * The only thing this 
   * constructor does, is to call
   * <code>checkCreateClassLoader</code> on the current 
   * security manager. 
   * @exception java.lang.SecurityException if not allowed
   * @since 1.2
   */
  protected ClassLoader(ClassLoader parent) 
  {
    SecurityManager security = System.getSecurityManager ();
    if (security != null)
      security.checkCreateClassLoader ();
    this.parent = parent;
  }

  /** 
   * Loads and link the class by the given name.
   * @param     name the name of the class.
   * @return    the class loaded.
   * @see       ClassLoader#loadClass(String,boolean)
   * @exception java.lang.ClassNotFoundException 
   */ 
  public Class loadClass(String name)
    throws java.lang.ClassNotFoundException
  { 
    return loadClass (name, false);
  }
  
  /** 
   * Loads the class by the given name.  The default implementation
   * will search for the class in the following order (similar to jdk 1.2)
   * <ul>
   *  <li> First <code>findLoadedClass</code>.
   *  <li> If parent is non-null, <code>parent.loadClass</code>;
   *       otherwise <code>findSystemClass</code>.
   *  <li> <code>findClass</code>.
   * </ul>
   * If <code>link</code> is true, <code>resolveClass</code> is then
   * called.  <p> Normally, this need not be overridden; override
   * <code>findClass</code> instead.
   * @param     name the name of the class.
   * @param     link if the class should be linked.
   * @return    the class loaded.
   * @exception java.lang.ClassNotFoundException 
   * @deprecated 
   */ 
  protected Class loadClass(String name, boolean link)
    throws java.lang.ClassNotFoundException
  {
    Class c = findLoadedClass (name);

    if (c == null)
      {
	try {
	  if (parent != null)
	    return parent.loadClass (name, link);
	  else
	    c = gnu.gcj.runtime.VMClassLoader.instance.findClass (name);
	} catch (ClassNotFoundException ex) {
	  /* ignore, we'll try findClass */;
	}
      }

    if (c == null)
      c = findClass (name);

    if (c == null)
      throw new ClassNotFoundException (name);

    if (link)
      resolveClass (c);

    return c;
  }

  /** Find a class.  This should be overridden by subclasses; the
   *  default implementation throws ClassNotFoundException.
   *
   * @param name Name of the class to find.
   * @return     The class found.
   * @exception  java.lang.ClassNotFoundException
   * @since 1.2
   */
  protected Class findClass (String name)
    throws ClassNotFoundException
  {
    throw new ClassNotFoundException (name);
  }

  // Protection Domain definitions 
  // FIXME: should there be a special protection domain used for native code?
  
  // The permission required to check what a classes protection domain is.
  static final Permission protectionDomainPermission
    = new RuntimePermission("getProtectionDomain");
  // The protection domain returned if we cannot determine it. 
  static ProtectionDomain unknownProtectionDomain;
  // Protection domain to use when a class is defined without one specified.
  static ProtectionDomain defaultProtectionDomain;

  static
  {
    Permissions permissions = new Permissions();
    permissions.add(new AllPermission());
    unknownProtectionDomain = new ProtectionDomain(null, permissions);  

    CodeSource cs = new CodeSource(null, null);
    defaultProtectionDomain =
      new ProtectionDomain(cs, Policy.getPolicy().getPermissions(cs));
  }

  /** 
   * Defines a class, given the class-data.  According to the JVM, this
   * method should not be used; instead use the variant of this method
   * in which the name of the class being defined is specified
   * explicitly.   
   * <P>
   * If the name of the class, as specified (implicitly) in the class
   * data, denotes a class which has already been loaded by this class
   * loader, an instance of
   * <code>java.lang.ClassNotFoundException</code> will be thrown.
   *
   * @param     data    bytes in class file format.
   * @param     off     offset to start interpreting data.
   * @param     len     length of data in class file.
   * @return    the class defined.
   * @exception java.lang.ClassNotFoundException 
   * @exception java.lang.LinkageError
   * @see ClassLoader#defineClass(String,byte[],int,int) */
  protected final Class defineClass(byte[] data, int off, int len) 
    throws ClassFormatError
  {
    return defineClass (null, data, off, len, defaultProtectionDomain);
  }

  protected final Class defineClass(String name, byte[] data, int off, int len)
    throws ClassFormatError
  {
    return defineClass (name, data, off, len, defaultProtectionDomain);
  }
  
  /** 
   * Defines a class, given the class-data.  This is preferable
   * over <code>defineClass(byte[],off,len)</code> since it is more
   * secure.  If the expected name does not match that of the class
   * file, <code>ClassNotFoundException</code> is thrown.  If
   * <code>name</code> denotes the name of an already loaded class, a
   * <code>LinkageError</code> is thrown.
   * <p>
   * 
   * FIXME: How do we assure that the class-file data is not being
   * modified, simultaneously with the class loader running!?  If this
   * was done in some very clever way, it might break security.  
   * Right now I am thinking that defineclass should make sure never to
   * read an element of this array more than once, and that that would
   * assure the ``immutable'' appearance.  It is still to be determined
   * if this is in fact how defineClass operates.
   *
   * @param     name    the expected name.
   * @param     data    bytes in class file format.
   * @param     off     offset to start interpreting data.
   * @param     len     length of data in class file.
   * @param     protectionDomain security protection domain for the class.
   * @return    the class defined.
   * @exception java.lang.ClassNotFoundException 
   * @exception java.lang.LinkageError
   */
  protected final synchronized Class defineClass(String name,
						 byte[] data,
						 int off,
						 int len,
						 ProtectionDomain protectionDomain)
    throws ClassFormatError
  {
    if (data==null || data.length < off+len || off<0 || len<0)
      throw new ClassFormatError ("arguments to defineClass "
				  + "are meaningless");

    // as per 5.3.5.1
    if (name != null  &&  findLoadedClass (name) != null)
      throw new java.lang.LinkageError ("class " 
					+ name 
					+ " already loaded");
    
    if (protectionDomain == null)
      protectionDomain = defaultProtectionDomain;

    try {
      // Since we're calling into native code here, 
      // we better make sure that any generated
      // exception is to spec!

      return defineClass0 (name, data, off, len, protectionDomain);

    } catch (LinkageError x) {
      throw x;		// rethrow

    } catch (java.lang.VirtualMachineError x) {
      throw x;		// rethrow

    } catch (java.lang.Throwable x) {
      // This should never happen, or we are beyond spec.  
      
      throw new InternalError ("Unexpected exception "
			       + "while defining class "
			       + name + ": " 
			       + x.toString ());
    }
  }

  /** This is the entry point of defineClass into the native code */
  private native Class defineClass0 (String name,
				     byte[] data,
				     int off,
				     int len,
				     ProtectionDomain protectionDomain)
    throws ClassFormatError;

  /** 
   * Link the given class.  This will bring the class to a state where
   * the class initializer can be run.  Linking involves the following
   * steps: 
   * <UL>
   * <LI>  Prepare (allocate and internalize) the constant strings that
   *       are used in this class.
   * <LI>  Allocate storage for static fields, and define the layout
   *       of instance fields.
   * <LI>  Perform static initialization of ``static final'' int,
   *       long, float, double and String fields for which there is a
   *       compile-time constant initializer.
   * <LI>  Create the internal representation of the ``vtable''.
   * </UL>
   * For <code>gcj</code>-compiled classes, only the first step is
   * performed.  The compiler will have done the rest already.
   * <P>
   * This is called by the system automatically,
   * as part of class initialization; there is no reason to ever call
   * this method directly.  
   * <P> 
   * For historical reasons, this method has a name which is easily
   * misunderstood.  Java classes are never ``resolved''.  Classes are
   * linked; whereas method and field references are resolved.
   *
   * @param     clazz the class to link.
   * @exception java.lang.LinkageError
   */
  protected final void resolveClass(Class clazz)
  {
    resolveClass0(clazz);
  }

  static void resolveClass0(Class clazz)
  {
    synchronized (clazz)
      {
	try {
	  linkClass0 (clazz);
	} catch (Throwable x) {
	  markClassErrorState0 (clazz);

	  if (x instanceof Error)
	    throw (Error)x;
	  else    
	    throw new java.lang.InternalError
	      ("unexpected exception during linking: " + x);
	}
      }
  }

  /** Internal method.  Calls _Jv_PrepareClass and
   * _Jv_PrepareCompiledClass.  This is only called from resolveClass.  */ 
  private static native void linkClass0(Class clazz);

  /** Internal method.  Marks the given clazz to be in an erroneous
   * state, and calls notifyAll() on the class object.  This should only
   * be called when the caller has the lock on the class object.  */
  private static native void markClassErrorState0(Class clazz);

  /**
   * Defines a new package and creates a Package object.
   * The package should be defined before any class in the package is
   * defined with <code>defineClass()</code>. The package should not yet
   * be defined before in this classloader or in one of its parents (which
   * means that <code>getPackage()</code> should return <code>null</code>).
   * All parameters except the <code>name</code> of the package may be
   * <code>null</code>.
   * <p>
   * Subclasses should call this method from their <code>findClass()</code>
   * implementation before calling <code>defineClass()</code> on a Class
   * in a not yet defined Package (which can be checked by calling
   * <code>getPackage()</code>).
   *
   * @param name The name of the Package
   * @param specTitle The name of the specification
   * @param specVendor The name of the specification designer
   * @param specVersion The version of this specification
   * @param implTitle The name of the implementation
   * @param implVendor The vendor that wrote this implementation
   * @param implVersion The version of this implementation
   * @param sealed If sealed the origin of the package classes
   * @return the Package object for the specified package
   *
   * @exception IllegalArgumentException if the package name is null or if
   * it was already defined by this classloader or one of its parents.
   *
   * @see Package
   * @since 1.2
   */
  protected Package definePackage(String name,
				  String specTitle, String specVendor,
				  String specVersion, String implTitle,
				  String implVendor, String implVersion,
				  URL sealed)
  {
    if (getPackage(name) != null)
      throw new IllegalArgumentException("Package " + name
					 + " already defined");
    Package p = new Package(name,
			    specTitle, specVendor, specVersion,
			    implTitle, implVendor, implVersion,
			    sealed);
    synchronized (definedPackages)
    {
      definedPackages.put(name, p);
    }
    return p;
  }

  /**
   * Returns the Package object for the requested package name. It returns
   * null when the package is not defined by this classloader or one of its
   * parents.
   *
   * @since 1.2
   */
  protected Package getPackage(String name)
  {
    Package p;
    if (parent == null)
      // XXX - Should we use the bootstrap classloader?
      p = null;
    else
      p = parent.getPackage(name);

    if (p == null)
      {
        synchronized (definedPackages)
	{
	  p = (Package) definedPackages.get(name);
	}
      }

    return p;
  }

  /**
   * Returns all Package objects defined by this classloader and its parents.
   *
   * @since 1.2
   */
  protected Package[] getPackages()
  {
    Package[] allPackages;

    // Get all our packages.
    Package[] packages;
    synchronized(definedPackages)
    {
      packages = new Package[definedPackages.size()];
      definedPackages.values().toArray(packages);
    }

    // If we have a parent get all packages defined by our parents.
    if (parent != null)
      {
	Package[] parentPackages = parent.getPackages();
	allPackages = new Package[parentPackages.length + packages.length];
	System.arraycopy(parentPackages, 0, allPackages, 0,
			 parentPackages.length);
	System.arraycopy(packages, 0, allPackages, parentPackages.length,
			 packages.length);
      }
    else
      // XXX - Should we use the bootstrap classloader?
      allPackages = packages;

    return allPackages;
  }

  /** 
   * Returns a class found in a system-specific way, typically
   * via the <code>java.class.path</code> system property.  Loads the 
   * class if necessary.
   *
   * @param     name the class to resolve.
   * @return    the class loaded.
   * @exception java.lang.LinkageError 
   * @exception java.lang.ClassNotFoundException 
   */
  protected final Class findSystemClass(String name) 
    throws java.lang.ClassNotFoundException
  {
    return gnu.gcj.runtime.VMClassLoader.instance.loadClass (name);
  }

  /*
   * Does currently nothing. FIXME.
   */ 
  protected final void setSigners(Class claz, Object[] signers) {
    /* claz.setSigners (signers); */
  }

  /**
   * If a class named <code>name</code> was previously loaded using
   * this <code>ClassLoader</code>, then it is returned.  Otherwise
   * it returns <code>null</code>.  (Unlike the JDK this is native,
   * since we implement the class table internally.)
   * @param     name  class to find.
   * @return    the class loaded, or null.
   */ 
  protected final native Class findLoadedClass(String name);

  public static InputStream getSystemResourceAsStream(String name) {
    return getSystemClassLoader().getResourceAsStream (name);
  }

  public static URL getSystemResource(String name) {
    return getSystemClassLoader().getResource (name);
  }

  /**
   *   Return an InputStream representing the resource name.  
   *   This is essentially like 
   *   <code>getResource(name).openStream()</code>, except
   *   it masks out any IOException and returns null on failure.
   * @param   name  resource to load
   * @return  an InputStream, or null
   * @see     java.lang.ClassLoader#getResource(String)
   * @see     java.io.InputStream
   */
  public InputStream getResourceAsStream(String name) 
  {
    try {
      URL res = getResource (name);
      if (res == null) return null;
      return res.openStream ();
    } catch (java.io.IOException x) {
      return null;
    }
  }
 
  /**
   * Return an java.io.URL representing the resouce <code>name</code>.  
   * The default implementation just returns <code>null</code>.
   * @param   name  resource to load
   * @return  a URL, or null if there is no such resource.
   * @see     java.lang.ClassLoader#getResourceAsBytes(String)
   * @see     java.lang.ClassLoader#getResourceAsStream(String)
   * @see     java.io.URL
   */
  public URL getResource (String name) 
  {
    // The rules say search the parent class if non-null,
    // otherwise search the built-in class loader (assumed to be
    // the system ClassLoader).  If not found, call
    // findResource().
    URL result = null;

    ClassLoader delegate = parent;

    if (delegate == null)
      delegate = getSystemClassLoader ();
	
    // Protect ourselves from looping.
    if (this != delegate)
      result = delegate.getResource (name);

    if (result != null)
      return result;
    else
      return findResource (name);
  }

  protected URL findResource (String name)
  {
    // Default to returning null.  Derived classes implement this.
    return null;
  }

  public final Enumeration getResources (String name) throws IOException
  {
    // The rules say search the parent class if non-null,
    // otherwise search the built-in class loader (assumed to be
    // the system ClassLoader).  If not found, call
    // findResource().
    Enumeration result = null;

    ClassLoader delegate = parent;

    if (delegate == null)
      delegate = getSystemClassLoader ();
	
    // Protect ourselves from looping.
    if (this != delegate)
      result = delegate.getResources (name);

    if (result != null)
      return result;
    else
      return findResources (name);
  }

  protected Enumeration findResources (String name) throws IOException
  {
    // Default to returning null.  Derived classes implement this.
    return null;
  }
}
