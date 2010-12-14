

system.print("\n\n\nSTARTING A CAMERA OBJECT\n\n\n");

function callback(msg)
{
  system.print("\n\n"); 
  system.print(msg);
  system.print("\n\n"); 
}

function timer_callback()
{
  if(chat)
  {
    chat.invoke("write", "Hey brother!!" );
  }
  else
  {
    print("no chat window");
  }

}

sim = system.presences[0].runSimulation("ogregraphics");
chat = sim.invoke("getChatWindow");

if(chat)
{
  print("\n\nGot a chat window\n\n");
}

chat.invoke("bind", "eventname", callback);

system.timeout(60, null, timer_callback);


