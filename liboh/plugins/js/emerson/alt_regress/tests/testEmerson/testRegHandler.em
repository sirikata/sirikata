function test()
{
  a <- b;
  
  print("\nPassed 1\n");
  
  a(i) <- b;
  
  print("\nPassed 2\n");

  a <- b <- c;

  print("\nPassed 3\n");

  a(i) <- b <- c;

  print("\nPassed 4\n");

  a <- b <- c(i);
  
  print("\nPassed 5\n");

  a(i) <- b <- c(j);

  print("\nPassed 6\n");

}

test();
