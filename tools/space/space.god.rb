# A simple god script to keep a space server running, recovering from
# crashes and reporting errors.
#
# To use, create a config file, as described below to give basic setup info, and
# place it alongside this script.  Then run god with this file, e.g.
# > god -c /path/to/space.god.rb
#

# Fill in a configuration file, e.g. config.god.rb that looks something like this:
#
#SIRIKATA_ROOT = "/path/to/sirikata"
#SPACE_PORT = 7777
#SIRIKATA_UID = 'my_username'
## Optional: If you want emails, fill this in
#SIRIKATA_OWNER = 'myname'
#SIRIKATA_OWNER_EMAIL = 'me@example.com'
#SIRIKATA_SYSTEM_EMAIL = 'god@example.com'
## Optional: If you want to cap the memory usage, fill in
#SIRIKATA_MAX_MEMORY = 2.gigabytes
#
# Then replace this path with the path to your config file.
SIRIKATA_CONFIG = '/path/to/user/config.god.rb'
God.load(SIRIKATA_CONFIG)

SIRIKATA_BIN_DIR = File.join(SIRIKATA_ROOT, "build/cmake")
SIRIKATA_SPACE_BIN = File.join(SIRIKATA_BIN_DIR, "space_d")

# God automatically daemonizes Sirikata for us. This just keeps the pid files in
# the code directory so we can clean up easily.
God.pid_file_directory = SIRIKATA_BIN_DIR

if defined? SIRIKATA_OWNER and defined? SIRIKATA_OWNER_EMAIL and defined? SIRIKATA_SYSTEM_EMAIL
  God::Contacts::Email.defaults do |d|
    d.from_email = SIRIKATA_SYSTEM_EMAIL
    d.from_name = 'God'
    d.delivery_method = :smtp
  end
  God.contact(:email) do |c|
    c.name = SIRIKATA_OWNER
    c.group = 'developers'
    c.to_email = SIRIKATA_OWNER_EMAIL
  end
end

God.watch do |w|
  w.name = "sirikata-space-server"
  w.interval = 60.seconds
  w.start = "#{SIRIKATA_SPACE_BIN} --servermap=local --servermap-options=--port=#{SPACE_PORT}"
  w.grace = 20.seconds # 5 more than the default kill timer
  w.stop_timeout = 20.seconds # 5 more than the default kill timer
  w.uid = SIRIKATA_UID

  w.log = File.join(SIRIKATA_BIN_DIR, "god.log")
  #w.errlog = File.join(SIRIKATA_BIN_DIR, "god.err.log")

  w.dir = SIRIKATA_BIN_DIR

  # determine the state on startup
  w.transition(:init, { true => :up, false => :start }) do |on|
    on.condition(:process_running) do |c|
      c.running = true
    end
  end

  # determine when process has finished starting
  w.transition([:start, :restart], :up) do |on|
    on.condition(:process_running) do |c|
      c.running = true
    end

    # failsafe
    on.condition(:tries) do |c|
      c.times = 2
      c.transition = :start
    end
  end

  # start if process is not running
  w.transition(:up, :start) do |on|
    on.condition(:process_exits) do |c|
      if defined? SIRIKATA_OWNER
        c.notify = SIRIKATA_OWNER
      end
    end
  end

  # restart if memory or cpu is too high
  w.transition(:up, :restart) do |on|
    if defined? SIRIKATA_MAX_MEMORY
      on.condition(:memory_usage) do |c|
        c.above = SIRIKATA_MAX_MEMORY
        c.times = [3, 5]
      end
    end
  end

  # lifecycle
  w.lifecycle do |on|
    on.condition(:flapping) do |c|
      c.to_state = [:start, :restart]
      c.times = 5
      c.within = 5.minute
      c.transition = :unmonitored
      c.retry_in = 10.minutes
      c.retry_times = 5
      c.retry_within = 2.hours
    end
  end

end
