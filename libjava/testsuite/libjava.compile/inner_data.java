// Test referencing protected data from parent of inner class.

import java.util.Random;

public class inner_data
{
    private class Randomer extends Random {
	public long xxx ()
	{
	    return seed;
	}
    }
}

