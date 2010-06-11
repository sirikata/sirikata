use Net::SMTP;

print "Content-type: text/html\n\n";
             
$smtp = Net::SMTP->new("smtp.stanford.edu", Debug => 1)
    or die "$0: unable to connect to smtp server\n";

# Set sender
$smtp->mail($ARGV[0]);

# Set recipient
$smtp->to($ARGV[1]);

$smtp->subject("Craq Down!");


# Set body 
$smtp->data("Quick, somebody 1) go to meru28 and run killall java.  Then, run the new_craq_begin.sh script with *all* the cluster nodes in their .cluster file enabled.");

$smtp->quit();

print "Mail sent";
