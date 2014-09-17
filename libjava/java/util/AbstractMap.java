/* AbstractMap.java -- Abstract implementation of most of Map
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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


package java.util;

/**
 * An abstract implementation of Map to make it easier to create your own
 * implementations. In order to create an unmodifiable Map, subclass
 * AbstractMap and implement the <code>entrySet</code> (usually via an
 * AbstractSet).  To make it modifiable, also implement <code>put</code>,
 * and have <code>entrySet().iterator()</code> support <code>remove</code>.
 * <p>
 *
 * It is recommended that classes which extend this support at least the
 * no-argument constructor, and a constructor which accepts another Map.
 * Further methods in this class may be overridden if you have a more
 * efficient implementation.
 *
 * @author Original author unknown
 * @author Bryce McKinlay
 * @author Eric Blake <ebb9@email.byu.edu>
 * @see Map
 * @see Collection
 * @see HashMap
 * @see LinkedHashMap
 * @see TreeMap
 * @see WeakHashMap
 * @see IdentityHashMap
 * @since 1.2
 * @status updated to 1.4
 */
public abstract class AbstractMap implements Map
{
  /** An "enum" of iterator types. */
  // Package visible for use by subclasses.
  static final int KEYS = 0,
                   VALUES = 1,
                   ENTRIES = 2;

  /**
   * The cache for {@link #keySet()}.
   */
  // Package visible for use by subclasses.
  Set keys;

  /**
   * The cache for {@link #values()}.
   */
  // Package visible for use by subclasses.
  Collection values;

  /**
   * The main constructor, for use by subclasses.
   */
  protected AbstractMap()
  {
  }

  /**
   * Remove all entries from this Map (optional operation). This default
   * implementation calls entrySet().clear(). NOTE: If the entry set does
   * not permit clearing, then this will fail, too. Subclasses often
   * override this for efficiency.  Your implementation of entrySet() should
   * not call <code>AbstractMap.clear</code> unless you want an infinite loop.
   *
   * @throws UnsupportedOperationException if <code>entrySet().clear()</code>
   *         does not support clearing.
   * @see Set#clear()
   */
  public void clear()
  {
    entrySet().clear();
  }

  /**
   * Create a shallow copy of this Map, no keys or values are copied. The
   * default implementation simply calls <code>super.clone()</code>.
   *
   * @return the shallow clone
   * @throws CloneNotSupportedException if a subclass is not Cloneable
   * @see Cloneable
   * @see Object#clone()
   */
  protected Object clone() throws CloneNotSupportedException
  {
    AbstractMap copy = (AbstractMap) super.clone();
    // Clear out the caches; they are stale.
    copy.keys = null;
    copy.values = null;
    return copy;
  }

  /**
   * Returns true if this contains a mapping for the given key. This
   * implementation does a linear search, O(n), over the
   * <code>entrySet()</code>, returning <code>true</code> if a match
   * is found, <code>false</code> if the iteration ends. Many subclasses
   * can implement this more efficiently.
   *
   * @param key the key to search for
   * @return true if the map contains the key
   * @throws NullPointerException if key is <code>null</code> but the map
   *         does not permit null keys
   * @see #containsValue(Object)
   */
  public boolean containsKey(Object key)
  {
    Iterator entries = entrySet().iterator();
    int pos = size();
    while (--pos >= 0)
      if (equals(key, ((Map.Entry) entries.next()).getKey()))
        return true;
    return false;
  }

  /**
   * Returns true if this contains at least one mapping with the given value.
   * This implementation does a linear search, O(n), over the
   * <code>entrySet()</code>, returning <code>true</code> if a match
   * is found, <code>false</code> if the iteration ends. A match is
   * defined as <code>(value == null ? v == null : value.equals(v))</code>
   * Subclasses are unlikely to implement this more efficiently.
   *
   * @param value the value to search for
   * @return true if the map contains the value
   * @see #containsKey(Object)
   */
  public boolean containsValue(Object value)
  {
    Iterator entries = entrySet().iterator();
    int pos = size();
    while (--pos >= 0)
      if (equals(value, ((Map.Entry) entries.next()).getValue()))
        return true;
    return false;
  }

  /**
   * Returns a set view of the mappings in this Map.  Each element in the
   * set must be an implementation of Map.Entry.  The set is backed by
   * the map, so that changes in one show up in the other.  Modifications
   * made while an iterator is in progress cause undefined behavior.  If
   * the set supports removal, these methods must be valid:
   * <code>Iterator.remove</code>, <code>Set.remove</code>,
   * <code>removeAll</code>, <code>retainAll</code>, and <code>clear</code>.
   * Element addition is not supported via this set.
   *
   * @return the entry set
   * @see Map.Entry
   */
  public abstract Set entrySet();

  /**
   * Compares the specified object with this map for equality. Returns
   * <code>true</code> if the other object is a Map with the same mappings,
   * that is,<br>
   * <code>o instanceof Map && entrySet().equals(((Map) o).entrySet();</code>
   *
   * @param o the object to be compared
   * @return true if the object equals this map
   * @see Set#equals(Object)
   */
  public boolean equals(Object o)
  {
    return (o == this ||
            (o instanceof Map &&
             entrySet().equals(((Map) o).entrySet())));
  }

  /**
   * Returns the value mapped by the given key. Returns <code>null</code> if
   * there is no mapping.  However, in Maps that accept null values, you
   * must rely on <code>containsKey</code> to determine if a mapping exists.
   * This iteration takes linear time, searching entrySet().iterator() of
   * the key.  Many implementations override this method.
   *
   * @param key the key to look up
   * @return the value associated with the key, or null if key not in map
   * @throws NullPointerException if this map does not accept null keys
   * @see #containsKey(Object)
   */
  public Object get(Object key)
  {
    Iterator entries = entrySet().iterator();
    int pos = size();
    while (--pos >= 0)
      {
        Map.Entry entry = (Map.Entry) entries.next();
        if (equals(key, entry.getKey()))
          return entry.getValue();
      }
    return null;
  }

  /**
   * Returns the hash code for this map. As defined in Map, this is the sum
   * of all hashcodes for each Map.Entry object in entrySet, or basically
   * entrySet().hashCode().
   *
   * @return the hash code
   * @see Map.Entry#hashCode()
   * @see Set#hashCode()
   */
  public int hashCode()
  {
    return entrySet().hashCode();
  }

  /**
   * Returns true if the map contains no mappings. This is implemented by
   * <code>size() == 0</code>.
   *
   * @return true if the map is empty
   * @see #size()
   */
  public boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Returns a set view of this map's keys. The set is backed by the map,
   * so changes in one show up in the other. Modifications while an iteration
   * is in progress produce undefined behavior. The set supports removal
   * if entrySet() does, but does not support element addition.
   * <p>
   *
   * This implementation creates an AbstractSet, where the iterator wraps
   * the entrySet iterator, size defers to the Map's size, and contains
   * defers to the Map's containsKey. The set is created on first use, and
   * returned on subsequent uses, although since no synchronization occurs,
   * there is a slight possibility of creating two sets.
   *
   * @return a Set view of the keys
   * @see Set#iterator()
   * @see #size()
   * @see #containsKey(Object)
   * @see #values()
   */
  public Set keySet()
  {
    if (keys == null)
      keys = new AbstractSet()
      {
        public int size()
        {
          return AbstractMap.this.size();
        }

        public boolean contains(Object key)
        {
          return containsKey(key);
        }

        public Iterator iterator()
        {
          return new Iterator()
          {
            private final Iterator map_iterator = entrySet().iterator();

            public boolean hasNext()
            {
              return map_iterator.hasNext();
            }

            public Object next()
            {
              return ((Map.Entry) map_iterator.next()).getKey();
            }

            public void remove()
            {
              map_iterator.remove();
            }
          };
        }
      };
    return keys;
  }

  /**
   * Associates the given key to the given value (optional operation). If the
   * map already contains the key, its value is replaced. This implementation
   * simply throws an UnsupportedOperationException. Be aware that in a map
   * that permits <code>null</code> values, a null return does not always
   * imply that the mapping was created.
   *
   * @param key the key to map
   * @param value the value to be mapped
   * @return the previous value of the key, or null if there was no mapping
   * @throws UnsupportedOperationException if the operation is not supported
   * @throws ClassCastException if the key or value is of the wrong type
   * @throws IllegalArgumentException if something about this key or value
   *         prevents it from existing in this map
   * @throws NullPointerException if the map forbids null keys or values
   * @see #containsKey(Object)
   */
  public Object put(Object key, Object value)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Copies all entries of the given map to this one (optional operation). If
   * the map already contains a key, its value is replaced. This implementation
   * simply iterates over the map's entrySet(), calling <code>put</code>,
   * so it is not supported if puts are not.
   *
   * @param m the mapping to load into this map
   * @throws UnsupportedOperationException if the operation is not supported
   * @throws ClassCastException if a key or value is of the wrong type
   * @throws IllegalArgumentException if something about a key or value
   *         prevents it from existing in this map
   * @throws NullPointerException if the map forbids null keys or values, or
   *         if <code>m</code> is null.
   * @see #put(Object, Object)
   */
  public void putAll(Map m)
  {
    Iterator entries = m.entrySet().iterator();
    int pos = m.size();
    while (--pos >= 0)
      {
        Map.Entry entry = (Map.Entry) entries.next();
        put(entry.getKey(), entry.getValue());
      }
  }

  /**
   * Removes the mapping for this key if present (optional operation). This
   * implementation iterates over the entrySet searching for a matching
   * key, at which point it calls the iterator's <code>remove</code> method.
   * It returns the result of <code>getValue()</code> on the entry, if found,
   * or null if no entry is found. Note that maps which permit null values
   * may also return null if the key was removed.  If the entrySet does not
   * support removal, this will also fail. This is O(n), so many
   * implementations override it for efficiency.
   *
   * @param key the key to remove
   * @return the value the key mapped to, or null if not present
   * @throws UnsupportedOperationException if deletion is unsupported
   * @see Iterator#remove()
   */
  public Object remove(Object key)
  {
    Iterator entries = entrySet().iterator();
    int pos = size();
    while (--pos >= 0)
      {
        Map.Entry entry = (Map.Entry) entries.next();
        if (equals(key, entry.getKey()))
          {
            // Must get the value before we remove it from iterator.
            Object r = entry.getValue();
            entries.remove();
            return r;
          }
      }
    return null;
  }

  /**
   * Returns the number of key-value mappings in the map. If there are more
   * than Integer.MAX_VALUE mappings, return Integer.MAX_VALUE. This is
   * implemented as <code>entrySet().size()</code>.
   *
   * @return the number of mappings
   * @see Set#size()
   */
  public int size()
  {
    return entrySet().size();
  }

  /**
   * Returns a String representation of this map. This is a listing of the
   * map entries (which are specified in Map.Entry as being
   * <code>getKey() + "=" + getValue()</code>), separated by a comma and
   * space (", "), and surrounded by braces ('{' and '}'). This implementation
   * uses a StringBuffer and iterates over the entrySet to build the String.
   * Note that this can fail with an exception if underlying keys or
   * values complete abruptly in toString().
   *
   * @return a String representation
   * @see Map.Entry#toString()
   */
  public String toString()
  {
    Iterator entries = entrySet().iterator();
    StringBuffer r = new StringBuffer("{");
    for (int pos = size(); pos > 0; pos--)
      {
        Map.Entry entry = (Map.Entry) entries.next();
        r.append(entry.getKey());
        r.append('=');
        r.append(entry.getValue());
        if (pos > 1)
          r.append(", ");
      }
    r.append("}");
    return r.toString();
  }

  /**
   * Returns a collection or bag view of this map's values. The collection
   * is backed by the map, so changes in one show up in the other.
   * Modifications while an iteration is in progress produce undefined
   * behavior. The collection supports removal if entrySet() does, but
   * does not support element addition.
   * <p>
   *
   * This implementation creates an AbstractCollection, where the iterator
   * wraps the entrySet iterator, size defers to the Map's size, and contains
   * defers to the Map's containsValue. The collection is created on first
   * use, and returned on subsequent uses, although since no synchronization
   * occurs, there is a slight possibility of creating two collections.
   *
   * @return a Collection view of the values
   * @see Collection#iterator()
   * @see #size()
   * @see #containsValue(Object)
   * @see #keySet()
   */
  public Collection values()
  {
    if (values == null)
      values = new AbstractCollection()
      {
        public int size()
        {
          return AbstractMap.this.size();
        }

        public Iterator iterator()
        {
          return new Iterator()
          {
            private final Iterator map_iterator = entrySet().iterator();

            public boolean hasNext()
            {
              return map_iterator.hasNext();
            }

            public Object next()
            {
              return ((Map.Entry) map_iterator.next()).getValue();
            }

            public void remove()
            {
              map_iterator.remove();
            }
          };
        }
      };
    return values;
  }

  /**
   * Compare two objects according to Collection semantics.
   *
   * @param o1 the first object
   * @param o2 the second object
   * @return o1 == null ? o2 == null : o1.equals(o2)
   */
  // Package visible for use throughout java.util.
  // It may be inlined since it is final.
  static final boolean equals(Object o1, Object o2)
  {
    return o1 == null ? o2 == null : o1.equals(o2);
  }

  /**
   * Hash an object according to Collection semantics.
   *
   * @param o the object to hash
   * @return o1 == null ? 0 : o1.hashCode()
   */
  // Package visible for use throughout java.util.
  // It may be inlined since it is final.
  static final int hashCode(Object o)
  {
    return o == null ? 0 : o.hashCode();
  }
}
