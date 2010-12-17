/*
  THis is the code for the emerson library
*/


/* variables */

addressable = system.addressable;
Self = system.Self;
presences = system.presences;
scale = system.scale;
position = system.position;
velocity = system.velocity;
orientation = system.orientation;
angularAxis = system.angularAxis;
angularVelocity = system.angularVelocity;




/* Functions */
function matches(pat, str)
{
  var regex = new RegExp(pat);
  regex.compile(regex);
  return regex.test(str);  
}

function print(str)
{
  system.print(str);  
}

/* doubtfull if this works */
function Pattern(name, value)
{
  p = new system.Pattern(name, value);
  this.name = p.name;
  this.value = p.value;
  
}

/* broadcast to all the addressables */
function broadcast(msg)
{
  print(addressable.length);  
  for(var i = 0; i < addressable.length; i++)
  {
    
    print("Sedning b4 to " + addressable[i]);
    if(addressable[i] != Self)
    {
      print("Sedning to " + addressable[i]);
      msg -> addressable[i];
    }
  }

  print("\n\n\n done with broadcast\n\n\n");
}


/* Import a file */
function import(file)
{
  system.import(file);
}

/* timeout */
function timeout(duration, target, cb)
{
  system.timeout(duration, target ,cb);
}

  
