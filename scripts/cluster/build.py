#!/usr/bin/python

import sys
import subprocess
import os
import os.path
import stat
from config import ClusterConfig
from run import ClusterRun, ClusterRunConcatCommands, ClusterRunFailed, ClusterRunSummaryCode
from scp import ClusterSCP

class Repository:
    def __init__(self, config, name, code_base, code_dir, is_submodule=False):
        self.config = config
        self.name = name
        self.code_base = code_base
        self.code_dir = code_dir
        self.patchmail_file = '.' + name + '.commits.patch'
        self.patch_file = '.' + name + '.changes.patch'
        self.is_submodule = is_submodule

    # Gets the path to the local directory for this repository
    def local_directory(self):
        # Assume we're in the tree somewhere.  Walk up until we find something that looks like
        sirikata_dir = os.getcwd()
        while True:
            if (os.path.exists(os.path.join(sirikata_dir, 'libcore')) and os.path.exists(os.path.join(sirikata_dir, 'liboh')) and os.path.exists(os.path.join(sirikata_dir, 'libspace'))):
                break
            sirikata_dir = os.path.dirname(sirikata_dir)
        return os.path.join(sirikata_dir, self.code_dir)

    def remote_directory(self):
        return os.path.join(self.code_base, self.code_dir)

    def cd_to_code(self):
        return "cd " + self.remote_directory()

    def apply_patch(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.remote_directory()+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "patch -p1 < " + patch_file
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, patch_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def apply_patch_mail(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.remote_directory()+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "git am " + patch_file
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, patch_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def update(self, with_submodules=True):
        cd_cmd = self.cd_to_code()
        pull_cmd = "git pull origin master"
        reset_cmd = ""
        if self.is_submodule:
            reset_cmd = "git checkout origin/master"
        submodules_init_cmd = ""
        submodules_update_cmd = ""
        if with_submodules:
            submodules_init_cmd = "git submodule init"
            submodules_update_cmd = "git submodule update"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, pull_cmd, reset_cmd, submodules_init_cmd, submodules_update_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def reset_to_origin_head(self):
        cd_cmd = self.cd_to_code()
        reset_cmd = "git reset --hard origin/" + self.config.branch
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, reset_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def git_clean(self):
        cd_cmd = self.cd_to_code()
        # note: we put clean_garbage before clean so the latter won't complain about these directories
        clean_garbage_cmd = "rm -rf .dotest" # FIXME there's probably other types of garbage
        clean_cmd = "git clean -f"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, clean_garbage_cmd, clean_cmd]))
        return ClusterRunSummaryCode(retcodes)

    # generates a patchset based on changes made to tree locally
    def create_patchset(self):
        # first generate a patchmail
        commits_patch_file = open(self.patchmail_file, 'w')
        # FIXME in submodules, do we always want to use the same branch as in the main repository
        rep_dir = self.local_directory()
        formatpatch_ret = subprocess.call(['git', 'format-patch', '--stdout', 'origin/' + self.config.branch], stdout=commits_patch_file, cwd=rep_dir)
        commits_patch_file.close()
        if (formatpatch_ret != 0):
            return formatpatch_ret

        # then generate a diff
        changes_patch_file = open(self.patch_file, 'w')
        diff_ret = subprocess.call(['git', 'diff'], stdout=changes_patch_file, cwd=rep_dir)
        changes_patch_file.close()
        return diff_ret

    # applies a patchset to all nodes
    def apply_patchset(self):
        # Setup all possible necessary commands, then filter out based on files
        file_cmds = [ ('git am', self.patchmail_file),
                      ('patch -p1 <', self.patch_file)
                      ]
        to_do = [ (cmd,file) for (cmd,file) in file_cmds if os.stat(file)[stat.ST_SIZE] > 0 ]

        # Copy files over
        scp_args = [ file for (cmd,file) in to_do ]
        scp_args.append("remote:"+self.remote_directory()+"/")
        ClusterSCP(self.config, scp_args)

        # Perform actual patching
        cd_cmd = self.cd_to_code()
        patch_cmds = [ (cmd + ' ' + file) for (cmd, file) in to_do ]

        cmds = [cd_cmd]
        cmds.extend(patch_cmds)

        retcodes = ClusterRun(self.config, ClusterRunConcatCommands(cmds))
        return ClusterRunSummaryCode(retcodes)

    # backs off all changes generated by a patchset
    # should leave you with a clean tree at origin/HEAD
    def revert_patchset(self):
        reset_ret = self.reset_to_origin_head()
        if (reset_ret != 0):
            return reset_ret

        clean_ret = self.git_clean()
        if (clean_ret != 0):
            return clean_ret

        update_ret = self.update(with_submodules=False)
        return update_ret


class ClusterBuild:
    def __init__(self, config):
        self.config = config
        # We only have 1 top level repository, but we list all of the
        # child repositories (submodules) so we can do patches for
        # everything.
        #
        # NOTE: These are listed from deeper to shallower
        # intentionally: this allows us to properly handle cleaning up
        # -- submodules are cleaned first, followed by the top-level
        # repository, avoiding errors due to modified submodules.
        self.repositories = [
            Repository(config, 'prox', self.config.code_dir, 'externals/prox', True),
            Repository(config, 'http-parser', self.config.code_dir, 'externals/http-parser', True),
            Repository(config, 'sirikata', self.config.code_dir, '')
            ]

    def cd_to_code(self):
        return "cd " + self.config.code_dir

    def cd_to_build(self):
        return "cd build/cmake"

    def cd_to_scripts(self):
        return "cd scripts"

    def destroy(self):
        destroy_cmd = "rm -rf " + self.config.code_dir
        retcodes = ClusterRun(self.config, destroy_cmd)
        return ClusterRunSummaryCode(retcodes)

    def checkout(self):
        checkout_cmd = "git clone " + self.config.repository + " " + self.config.code_dir
        cd_cmd = self.cd_to_code()
        branch_cmd = "git branch _cluster origin/"  + self.config.branch
        checkout_branch_cmd = "git checkout _cluster"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([checkout_cmd, cd_cmd, branch_cmd, checkout_branch_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def dependencies(self):
        cd_cmd = self.cd_to_code()
        build_cmd = "make minimal-depends"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, build_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def ccache_args(self):
        if (not self.config.ccache):
            return ''

        # FIXME caching this somewhere would probably be wise...
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands(["ls /usr/bin/ccache /usr/bin/g++ /usr/bin/gcc &> /dev/null"]))
        # FIXME we should do this per-node, but cluster_run doesn't support that yet for per-node runs
        if (ClusterRunSummaryCode(retcodes) == 0):
            # We have all the pieces we need
            return 'CC="/usr/bin/ccache /usr/bin/gcc" CXX="/usr/bin/ccache /usr/bin/g++"'

        print 'Warning: Running without ccache!'
        return ""

    def build(self, build_type = 'Default', with_timestamp = True):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        build_cmd = "%s cmake -DCMAKE_BUILD_TYPE=%s -DCBR_TIMESTAMP_PACKETS=%s ." % (self.ccache_args(), build_type, str(with_timestamp))
        make_cmd = "make -j2"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_build_cmd, build_cmd, make_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def clean(self):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        clean_cmd = "make clean "
        cache_cmd = "rm -f CMakeCache.txt";
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, clean_cmd, cd_build_cmd, cache_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def create_patchset(self):
        retcodes = []
        for rep in self.repositories:
            retcodes.append( rep.create_patchset() )
        return ClusterRunSummaryCode(retcodes)

    def apply_patchset(self):
        retcodes = []
        for rep in self.repositories:
            retcodes.append( rep.apply_patchset() );
        return ClusterRunSummaryCode(retcodes)

    def revert_patchset(self):
        retcodes = []
        for rep in self.repositories:
            retcodes.append( rep.revert_patchset() );
        return ClusterRunSummaryCode(retcodes)

    def profile(self, binary):
        cd_code_cmd = self.cd_to_code()
        cd_scripts_cmd = self.cd_to_scripts()
        prof_cmd = "gprof ../build/cmake/%s > gprof.out" % (binary)
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_scripts_cmd, prof_cmd]))

        gprof_pattern = "gprof-%(node)04d.txt"
        ClusterSCP(self.config, ["remote:"+self.config.code_dir+"/scripts/gprof.out", gprof_pattern])

        return ClusterRunSummaryCode(retcodes)

    def oprofile(self, binary):
        cd_code_cmd = self.cd_to_code()
        cd_scripts_cmd = self.cd_to_scripts()
        prof_cmd = "opreport \\*cbr\\* > oprofile.out"
        prof_cmd += "; opreport -l \\*cbr\\* >> oprofile.out"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_scripts_cmd, prof_cmd]))

        oprof_pattern = "oprofile-%(node)04d.txt"
        ClusterSCP(self.config, ["remote:"+self.config.code_dir+"/scripts/oprofile.out", oprof_pattern])

        return ClusterRunSummaryCode(retcodes)


if __name__ == "__main__":
    cc = ClusterConfig()
    cluster_build = ClusterBuild(cc)

    if len(sys.argv) < 2:
        print 'No command provided...'
        exit(-1)

    # Some commands are simple shorthands for others and can just be
    # expanded into their long forms
    filter_rules = {
        'deploy' : ['patchset_revert', 'patchset_create', 'patchset_apply', 'clean', 'build'],
        'update_dependencies' : ['dependencies']
        }
    filtered_args = []
    for arg in sys.argv:
        expanded = [arg]
        if arg in filter_rules: expanded = filter_rules[arg]
        filtered_args.extend(expanded)

    cur_arg_idx = 1
    while cur_arg_idx < len(filtered_args):
        cmd = filtered_args[cur_arg_idx]
        cur_arg_idx += 1
        retval = 0

        if cmd == 'destroy':
            retval = cluster_build.destroy()
        elif cmd == 'checkout':
            retval = cluster_build.checkout()
        elif cmd == 'update':
            retval = cluster_build.update()
        elif cmd == 'dependencies':
            deps = []
            retval = cluster_build.dependencies()
        elif cmd == 'build':
            build_type = 'Debug'
            with_timestamp = True
            tstamp_map = { 'timestamp' : True, 'no-timestamp' : False }
            while(cur_arg_idx < len(filtered_args)):
                if filtered_args[cur_arg_idx] in ['Debug', 'Release', 'RelWithDebInfo', 'Profile', 'Coverage']:
                    build_type = filtered_args[cur_arg_idx]
                    cur_arg_idx += 1
                elif filtered_args[cur_arg_idx] in tstamp_map.keys():
                    with_timestamp = tstamp_map[filtered_args[cur_arg_idx]]
                    cur_arg_idx += 1
                else:
                    break
            retval = cluster_build.build(build_type, with_timestamp)
        elif cmd == 'clean':
            retval = cluster_build.clean()
        elif cmd == 'fullbuild':
            retval = cluster_build.destroy()
            if (retval == 0):
                retval = cluster_build.checkout()
            if (retval == 0):
                retval = cluster_build.update()
            if (retval == 0):
                retval = cluster_build.dependencies()
            if (retval == 0):
                retval = cluster_build.build()
        elif cmd == 'patchset_create':
            retval = cluster_build.create_patchset()
        elif cmd == 'patchset_apply':
            retval = cluster_build.apply_patchset()
        elif cmd == 'patchset_revert':
            retval = cluster_build.revert_patchset()
        elif cmd == 'profile':
            profile_binary = 'cbr'
            if (cur_arg_idx < len(filtered_args) and filtered_args[cur_arg_idx] in ['cbr', 'simoh', 'cseg', 'analysis']):
                profile_binary = filtered_args[cur_arg_idx]
                cur_arg_idx += 1
            retval = cluster_build.profile(profile_binary)
        elif cmd == 'oprofile':
            profile_binary = 'cbr'
            if (cur_arg_idx < len(filtered_args) and filtered_args[cur_arg_idx] in ['cbr', 'simoh', 'cseg', 'analysis']):
                profile_binary = filtered_args[cur_arg_idx]
                cur_arg_idx += 1
            retval = cluster_build.oprofile(profile_binary)
        else:
            print "Unknown command: ", cmd
            exit(-1)

        if (retval != 0):
            print "Error while running command '", cmd, "', exiting"
            exit(-1)
