------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUNTIME COMPONENTS                          --
--                                                                          --
--            A D A . N U M E R I C S . F L O A T _ R A N D O M             --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-1998, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, --
-- MA 02111-1307, USA.                                                      --
--                                                                          --
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

with Ada.Calendar;

package body Ada.Numerics.Float_Random is

   -------------------------
   -- Implementation Note --
   -------------------------

   --  The design of this spec is very awkward, as a result of Ada 95 not
   --  permitting in-out parameters for function formals (most naturally
   --  Generator values would be passed this way). In pure Ada 95, the only
   --  solution is to use the heap and pointers, and, to avoid memory leaks,
   --  controlled types.

   --  This is awfully heavy, so what we do is to use Unrestricted_Access to
   --  get a pointer to the state in the passed Generator. This works because
   --  Generator is a limited type and will thus always be passed by reference.

   type Pointer is access all State;

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Euclid (P, Q : in Int; X, Y : out Int; GCD : out Int);

   function  Euclid (P, Q : Int) return Int;

   function Square_Mod_N (X, N : Int) return Int;

   ------------
   -- Euclid --
   ------------

   procedure Euclid (P, Q : in Int; X, Y : out Int; GCD : out Int) is

      XT : Int := 1;
      YT : Int := 0;

      procedure Recur
        (P,  Q  : in Int;                 --  a (i-1), a (i)
         X,  Y  : in Int;                 --  x (i),   y (i)
         XP, YP : in out Int;             --  x (i-1), y (i-1)
         GCD    : out Int);

      procedure Recur
        (P,  Q  : in Int;
         X,  Y  : in Int;
         XP, YP : in out Int;
         GCD    : out Int)
      is
         Quo : Int  := P / Q;             --  q <-- |_ a (i-1) / a (i) _|
         XT  : Int := X;                  --  x (i)
         YT  : Int := Y;                  --  y (i)

      begin
         if P rem Q = 0 then                 --  while does not divide
            GCD := Q;
            XP  := X;
            YP  := Y;
         else
            Recur (Q, P - Q * Quo, XP - Quo * X, YP - Quo * Y, XT, YT, Quo);

            --  a (i) <== a (i)
            --  a (i+1) <-- a (i-1) - q*a (i)
            --  x (i+1) <-- x (i-1) - q*x (i)
            --  y (i+1) <-- y (i-1) - q*y (i)
            --  x (i) <== x (i)
            --  y (i) <== y (i)

            XP  := XT;
            YP  := YT;
            GCD := Quo;
         end if;
      end Recur;

   --  Start of processing for Euclid

   begin
      Recur (P, Q, 0, 1, XT, YT, GCD);
      X := XT;
      Y := YT;
   end Euclid;

   function Euclid (P, Q : Int) return Int is
      X, Y, GCD : Int;

   begin
      Euclid (P, Q, X, Y, GCD);
      return X;
   end Euclid;

   -----------
   -- Image --
   -----------

   function Image (Of_State : State) return String is
   begin
      return Int'Image (Of_State.X1) & ',' & Int'Image (Of_State.X2)
             & ',' &
             Int'Image (Of_State.P)  & ',' & Int'Image (Of_State.Q);
   end Image;

   ------------
   -- Random --
   ------------

   function Random  (Gen : Generator) return Uniformly_Distributed is
      Genp : constant Pointer := Gen.Gen_State'Unrestricted_Access;

   begin
      Genp.X1 := Square_Mod_N (Genp.X1,  Genp.P);
      Genp.X2 := Square_Mod_N (Genp.X2,  Genp.Q);
      return
        Float ((Flt (((Genp.X2 - Genp.X1) * Genp.X)
                  mod Genp.Q) * Flt (Genp.P)
          + Flt (Genp.X1)) * Genp.Scl);
   end Random;

   -----------
   -- Reset --
   -----------

   --  Version that works from given initiator value

   procedure Reset (Gen : in Generator; Initiator : in Integer) is
      Genp   : constant Pointer := Gen.Gen_State'Unrestricted_Access;
      X1, X2 : Int;

   begin
      X1 := 2 + Int (Initiator) mod (K1 - 3);
      X2 := 2 + Int (Initiator) mod (K2 - 3);

      --  Eliminate effects of small Initiators.

      for J in 1 .. 5 loop
         X1 := Square_Mod_N (X1, K1);
         X2 := Square_Mod_N (X2, K2);
      end loop;

      Genp.all :=
        (X1  => X1,
         X2  => X2,
         P   => K1,
         Q   => K2,
         X   => 1,
         Scl => Scal);
   end Reset;

   --  Version that works from specific saved state

   procedure Reset (Gen : Generator; From_State : State) is
      Genp : constant Pointer := Gen.Gen_State'Unrestricted_Access;

   begin
      Genp.all := From_State;
   end Reset;

   --  Version that works from calendar

   procedure Reset (Gen : Generator) is
      Genp   : constant Pointer       := Gen.Gen_State'Unrestricted_Access;
      Now    : constant Calendar.Time := Calendar.Clock;
      X1, X2 : Int;

   begin
      X1 := Int (Calendar.Year  (Now)) * 12 * 31 +
            Int (Calendar.Month (Now)) * 31 +
            Int (Calendar.Day   (Now));

      X2 := Int (Calendar.Seconds (Now) * Duration (1000.0));

      X1 := 2 + X1 mod (K1 - 3);
      X2 := 2 + X2 mod (K2 - 3);

      --  Eliminate visible effects of same day starts

      for J in 1 .. 5 loop
         X1 := Square_Mod_N (X1, K1);
         X2 := Square_Mod_N (X2, K2);
      end loop;


      Genp.all :=
        (X1  => X1,
         X2  => X2,
         P   => K1,
         Q   => K2,
         X   => 1,
         Scl => Scal);

   end Reset;

   ----------
   -- Save --
   ----------

   procedure Save (Gen : in Generator; To_State : out State) is
   begin
      To_State := Gen.Gen_State;
   end Save;

   ------------------
   -- Square_Mod_N --
   ------------------

   function Square_Mod_N (X, N : Int) return Int is
      Temp : Flt := Flt (X) * Flt (X);
      Div  : Int := Int (Temp / Flt (N));

   begin
      Div := Int (Temp - Flt (Div) * Flt (N));

      if Div < 0 then
         return Div + N;
      else
         return Div;
      end if;
   end Square_Mod_N;

   -----------
   -- Value --
   -----------

   function Value (Coded_State : String) return State is
      Start : Positive := Coded_State'First;
      Stop  : Positive := Coded_State'First;
      Outs  : State;

   begin
      while Coded_State (Stop) /= ',' loop
         Stop := Stop + 1;
      end loop;

      Outs.X1 := Int'Value (Coded_State (Start .. Stop - 1));
      Start := Stop + 1;

      loop
         Stop := Stop + 1;
         exit when Coded_State (Stop) = ',';
      end loop;

      Outs.X2 := Int'Value (Coded_State (Start .. Stop - 1));
      Start := Stop + 1;

      loop
         Stop := Stop + 1;
         exit when Coded_State (Stop) = ',';
      end loop;

      Outs.P   := Int'Value (Coded_State (Start .. Stop - 1));
      Outs.Q   := Int'Value (Coded_State (Stop + 1 .. Coded_State'Last));
      Outs.X   := Euclid (Outs.P, Outs.Q);
      Outs.Scl := 1.0 / (Flt (Outs.P) * Flt (Outs.Q));

      --  Now do *some* sanity checks.

      if Outs.Q < 31 or else Outs.P < 31
        or else Outs.X1 not in 2 .. Outs.P - 1
        or else Outs.X2 not in 2 .. Outs.Q - 1
      then
         raise Constraint_Error;
      end if;

      return Outs;
   end Value;
end Ada.Numerics.Float_Random;
