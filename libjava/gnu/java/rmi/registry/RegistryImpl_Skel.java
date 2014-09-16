// Skel class generated by rmic - DO NOT EDIT!

package gnu.java.rmi.registry;

public final class RegistryImpl_Skel
    implements java.rmi.server.Skeleton
{
    private static final long interfaceHash = 4905912898345647071L;
    
    private static final java.rmi.server.Operation[] operations = {
        new java.rmi.server.Operation("void bind(java.lang.String, java.rmi.Remote"),
        new java.rmi.server.Operation("java.lang.String[] list("),
        new java.rmi.server.Operation("java.rmi.Remote lookup(java.lang.String"),
        new java.rmi.server.Operation("void rebind(java.lang.String, java.rmi.Remote"),
        new java.rmi.server.Operation("void unbind(java.lang.String")
    };
    
    public java.rmi.server.Operation[] getOperations() {
        return ((java.rmi.server.Operation[]) operations.clone());
    }
    
    public void dispatch(java.rmi.Remote obj, java.rmi.server.RemoteCall call, int opnum, long hash) throws java.lang.Exception {
        if (opnum < 0) {
            if (hash == 7583982177005850366L) {
                opnum = 0;
            }
            else if (hash == 2571371476350237748L) {
                opnum = 1;
            }
            else if (hash == -7538657168040752697L) {
                opnum = 2;
            }
            else if (hash == -8381844669958460146L) {
                opnum = 3;
            }
            else if (hash == 7305022919901907578L) {
                opnum = 4;
            }
            else {
                throw new java.rmi.server.SkeletonMismatchException("interface hash mismatch");
            }
        }
        else if (hash != interfaceHash) {
            throw new java.rmi.server.SkeletonMismatchException("interface hash mismatch");
        }
        
        gnu.java.rmi.registry.RegistryImpl server = (gnu.java.rmi.registry.RegistryImpl)obj;
        switch (opnum) {
        case 0:
        {
            java.lang.String $param_0;
            java.rmi.Remote $param_1;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.lang.String)in.readObject();
                $param_1 = (java.rmi.Remote)in.readObject();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            server.bind($param_0, $param_1);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        case 1:
        {
            try {
                java.io.ObjectInput in = call.getInputStream();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            java.lang.String[] $result = server.list();
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
                out.writeObject($result);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        case 2:
        {
            java.lang.String $param_0;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.lang.String)in.readObject();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            java.rmi.Remote $result = server.lookup($param_0);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
                out.writeObject($result);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        case 3:
        {
            java.lang.String $param_0;
            java.rmi.Remote $param_1;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.lang.String)in.readObject();
                $param_1 = (java.rmi.Remote)in.readObject();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            server.rebind($param_0, $param_1);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        case 4:
        {
            java.lang.String $param_0;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.lang.String)in.readObject();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            server.unbind($param_0);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        default:
            throw new java.rmi.UnmarshalException("invalid method number");
        }
    }
}
