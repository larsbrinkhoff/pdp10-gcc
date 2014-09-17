/*
  Copyright (c) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

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

package gnu.java.rmi.server;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.server.RemoteRef;
import java.rmi.server.RMISocketFactory;
import java.rmi.server.RMIClientSocketFactory;
import java.rmi.server.RMIServerSocketFactory;
import java.rmi.server.RemoteObject;
import java.rmi.server.RemoteCall;
import java.rmi.server.UnicastRemoteObject;
import java.rmi.server.Operation;
import java.rmi.server.ObjID;
import java.rmi.server.UID;
import java.lang.reflect.Method;
import java.io.ObjectOutput;
import java.io.ObjectInput;
import java.io.IOException;
import java.net.Socket;
import java.net.InetAddress;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;

public class UnicastRef
	implements RemoteRef, ProtocolConstants {

public ObjID objid;
UnicastConnectionManager manager;

/**
 * Used by serialization.
 */
private UnicastRef() {
}

public UnicastRef(ObjID objid, String host, int port, RMIClientSocketFactory csf) {
	this(objid);
	manager = UnicastConnectionManager.getInstance(host, port, csf);
}

public UnicastRef(ObjID objid) {
	this.objid = objid;
}

public Object invoke(Remote obj, Method method, Object[] params, long opnum) throws Exception {
	return (invokeCommon(obj, method, params, -1, opnum));
}

private Object invokeCommon(Remote obj, Method method, Object[] params, int opnum, long hash) throws Exception {
	UnicastConnection conn;
	try {
		conn = manager.getConnection();
	}
	catch (IOException e1) {
		throw new RemoteException("connection failed to host: " + manager.serverName, e1);
	}

	ObjectOutputStream out;
	DataOutputStream dout;
	try {
		dout = conn.getDataOutputStream();
		dout.writeByte(MESSAGE_CALL);

		out = conn.getObjectOutputStream();
		
		objid.write(out);
		out.writeInt(opnum);
		out.writeLong(hash);
		if (params != null) {
			for (int i = 0; i < params.length; i++) {
				if (params[i] instanceof UnicastRemoteObject) {
					out.writeObject(UnicastRemoteObject.exportObject((UnicastRemoteObject)params[i]));
				}
				else {
					out.writeObject(params[i]);
				}
			}
		}

		out.flush();
	}
	catch (IOException e2) {
		throw new RemoteException("call failed: ", e2);
	}

	int returncode;
	Object returnval;
	DataInputStream din;
	ObjectInputStream in;
	UID ack;
	try {
		din = conn.getDataInputStream();
		if (din.readUnsignedByte() != MESSAGE_CALL_ACK) {
			throw new RemoteException("Call not acked");
		}

		in = conn.getObjectInputStream();

		returncode = in.readUnsignedByte();
		ack = UID.read(in);
		returnval = in.readObject();
	}
	catch (IOException e3) {
		throw new RemoteException("call return failed: ", e3);
	}

	manager.discardConnection(conn);

	if (returncode != RETURN_ACK) {
		throw (Exception)returnval;
	}

	return (returnval);
}

/**
 * @deprecated
 */
public RemoteCall newCall(RemoteObject obj, Operation[] op, int opnum, long hash) throws RemoteException {
	return (new UnicastRemoteCall(obj, opnum, hash));
}

/**
 * @deprecated
 */
public void invoke(RemoteCall call) throws Exception {
	UnicastRemoteCall c = (UnicastRemoteCall)call;
	Object ret = invokeCommon((Remote)c.getObject(), (Method)null, c.getArguments(), c.getOpnum(), c.getHash());
	c.setReturnValue(ret);
}

/**
 * @deprecated
 */
public void done(RemoteCall call) throws RemoteException {
	/* Does nothing */
}

public void writeExternal(ObjectOutput out) throws IOException {
	if (manager == null) {
		throw new IOException("no connection");
	}
	manager.write(out);
	objid.write(out);
	out.writeByte(RETURN_ACK);
}

public void readExternal(ObjectInput in) throws IOException, ClassNotFoundException {
	manager = UnicastConnectionManager.read(in);
	objid = ObjID.read(in);
	if (in.readByte() != RETURN_ACK) {
		throw new IOException("no ack found");
	}
}

public boolean remoteEquals(RemoteRef ref) {
	throw new Error("Not implemented");
}

public int remoteHashCode() {
	throw new Error("Not implemented");
}

public String getRefClass(ObjectOutput out) {
	return ("UnicastRef");
}

public String remoteToString() {
	throw new Error("Not implemented");
}

public void dump(UnicastConnection conn) {
	try {
		DataInputStream din = conn.getDataInputStream();
		for (;;) {
			int b = din.readUnsignedByte();
			System.out.print(Integer.toHexString(b));
			if (b >= 32 && b < 128) {
				System.out.print(": " + (char)b);
			}
			System.out.println();
		}
	}
	catch (IOException _) {
	}
}

}
