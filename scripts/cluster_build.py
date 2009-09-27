#!/usr/bin/python

import sys
import subprocess
import os
import stat
from cluster_config import ClusterConfig
from cluster_run import ClusterRun, ClusterRunConcatCommands, ClusterRunFailed, ClusterRunSummaryCode
from cluster_scp import ClusterSCP

class ClusterBuild:
    def __init__(self, config):
        self.config = config

    def cd_to_code(self):
        return "cd " + self.config.code_dir

    def cd_to_build(self):
        return "cd build/cmake"

    def cd_to_sst_build(self):
        return ClusterRunConcatCommands( [self.cd_to_code(), "cd dependencies/sst"] )

    def cd_to_enet_build(self):
        return ClusterRunConcatCommands( [self.cd_to_code(), "cd dependencies/enet"] )

    def destroy(self):
        destroy_cmd = "rm -rf " + self.config.code_dir
        retcodes = ClusterRun(self.config, destroy_cmd)
        return ClusterRunSummaryCode(retcodes)

    def checkout(self):
        checkout_cmd = "git clone git@ahoy:cbr.git " + self.config.code_dir
        retcodes = ClusterRun(self.config, checkout_cmd)
        return ClusterRunSummaryCode(retcodes)

    def update(self):
        cd_cmd = self.cd_to_code()
        pull_cmd = "git pull origin"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, pull_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def apply_patch(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "patch -p1 < " + patch_file
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, patch_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def apply_patch_mail(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "git am " + patch_file
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, patch_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def reset_to_head(self):
        cd_cmd = self.cd_to_code()
        reset_cmd = "git reset --hard HEAD"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, reset_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def reset_to_origin_head(self):
        cd_cmd = self.cd_to_code()
        reset_cmd = "git reset --hard origin/HEAD"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, reset_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def git_clean(self):
        cd_cmd = self.cd_to_code()
        # note: we put clean_garbage before clean so the latter won't complain about these directories
        clean_garbage_cmd = "rm -rf .dotest" # FIXME there's probably other types of garbage
        clean_cmd = "git clean -f"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, clean_garbage_cmd, clean_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def dependencies(self, which = None):
        cd_cmd = self.cd_to_code()
        build_cmd = "./install-deps.sh"
        if which != None:
            for dep in which:
                build_cmd += " " + dep
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, build_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def update_dependencies(self, which = None):
        cd_cmd = self.cd_to_code()
        update_cmd = "./install-deps.sh update"
        if which != None:
            for dep in which:
                update_cmd += " " + dep
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, update_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def patch_build_sst(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/dependencies/sst/"+patch_file])
        cd_cmd = self.cd_to_sst_build()
        reset_cmd = "git reset --hard HEAD"
        patch_cmd = "patch -p1 < " + patch_file
        build_cmd = "make; make install"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, reset_cmd, patch_cmd, build_cmd]))
        # doing this implies we need to rebuild cbr
        if (ClusterRunFailed(retcodes)):
            return ClusterRunSummaryCode(retcodes)

        clean_ret = self.clean()
        if (clean_ret != 0):
            return clean_ret

        return self.build()


    def patch_build_enet(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/dependencies/enet/"+patch_file])
        cd_cmd = self.cd_to_enet_build()
        reset_cmd = "git reset --hard HEAD"
        patch_cmd = "patch -p1 < " + patch_file
        build_cmd = "make; make install"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, reset_cmd, patch_cmd, build_cmd]))
        # doing this implies we need to rebuild cbr
        if (ClusterRunFailed(retcodes)):
            return ClusterRunSummaryCode(retcodes)

        clean_ret = self.clean()
        if (clean_ret != 0):
            return clean_ret

        return self.build()


    def build(self, build_type):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        build_cmd = "cmake -DCMAKE_BUILD_TYPE=%s . && make -j2" % (build_type)
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_build_cmd, build_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def clean(self):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        clean_cmd = "make clean && rm CMakeCache.txt"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_build_cmd, clean_cmd]))
        return ClusterRunSummaryCode(retcodes)

    # generates a patchset based on changes made to tree locally
    def create_patchset(self):
        # first generate a patchmail
        commits_patch_file = open("commits.patch", 'w')
        formatpatch_ret = subprocess.call(['git', 'format-patch', '--stdout', 'origin/master'], 0, None, None, commits_patch_file)
        if (formatpatch_ret != 0):
            return formatpatch_ret

        # then generate a diff
        changes_patch_file = open("changes.patch", 'w')
        diff_ret = subprocess.call(['git', 'diff'], 0, None, None, changes_patch_file)
        changes_patch_file.close()
        return diff_ret

    # applies a patchset to all nodes
    def apply_patchset(self):
        # just use revert_patchset to make sure we're in a clean state
        revert_ret = self.revert_patchset()
        if (revert_ret != 0):
            return revert_ret

        # then apply the patchmail file
        if (os.stat('commits.patch')[stat.ST_SIZE] != 0):
            patchmail_ret = self.apply_patch_mail("commits.patch")
            if (patchmail_ret != 0):
                return patchmail_ret;

        # and finally the patch file
        if (os.stat('changes.patch')[stat.ST_SIZE] != 0):
            patch_ret = self.apply_patch("changes.patch")
            return patch_ret;

        return 0

    # backs off all changes generated by a patchset
    # should leave you with a clean tree at origin/HEAD
    def revert_patchset(self):
        reset_ret = self.reset_to_origin_head()
        if (reset_ret != 0):
            return reset_ret

        clean_ret = self.git_clean()
        if (clean_ret != 0):
            return clean_ret

        update_ret = self.update()
        return update_ret



if __name__ == "__main__":
    cc = ClusterConfig()
    cluster_build = ClusterBuild(cc)

    if len(sys.argv) < 2:
        print 'No command provided...'
        exit(-1)

    cur_arg_idx = 1
    while cur_arg_idx < len(sys.argv):
        cmd = sys.argv[cur_arg_idx]
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
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['sst', 'enet', 'sirikata', 'prox']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            retval = cluster_build.dependencies(deps)
        elif cmd == 'update_dependencies':
            deps = []
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['sst', 'enet', 'sirikata', 'prox']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            retval = cluster_build.update_dependencies(deps)
        elif cmd == 'patch_build_sst':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            retval = cluster_build.patch_build_sst(patch_file)
        elif cmd == 'patch_build_enet':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            retval = cluster_build.patch_build_enet(patch_file)
        elif cmd == 'build':
            build_type = 'Debug'
            if (sys.argv[cur_arg_idx] in ['Debug', 'Release', 'RelWithDebInfo']):
                build_type = sys.argv[cur_arg_idx]
                cur_arg_idx += 1
            retval = cluster_build.build(build_type)
        elif cmd == 'patch':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            retval = cluster_build.apply_patch(patch_file)
        elif cmd == 'patchmail':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            retval = cluster_build.apply_patch_mail(patch_file)
        elif cmd == 'reset':
            retval = cluster_build.reset_to_head()
        elif cmd == 'reset_origin':
            retval = cluster_build.reset_to_origin_head()
        elif cmd == 'git_clean':
            retval = cluster_build.git_clean()
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
                retval = cluster_build.update_dependencies()
            if (retval == 0):
                retval = cluster_build.build()
        elif cmd == 'patchset_create':
            retval = cluster_build.create_patchset()
        elif cmd == 'patchset_apply':
            retval = cluster_build.apply_patchset()
        elif cmd == 'patchset_revert':
            retval = cluster_build.revert_patchset()
        else:
            print "Unknown command: ", cmd
            exit(-1)

        if (retval != 0):
            print "Error while running command '", cmd, "', exiting"
            exit(-1)
