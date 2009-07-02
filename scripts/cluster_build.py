#!/usr/bin/python

import sys
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
        return ClusterRunConcatCommands( [cd_to_code(), "cd dependencies/sst"] )

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
        clean_cmd = "git clean -f"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_cmd, clean_cmd]))
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


    def build(self):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        build_cmd = "cmake . && make -j2"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_build_cmd, build_cmd]))
        return ClusterRunSummaryCode(retcodes)

    def clean(self):
        cd_code_cmd = self.cd_to_code()
        cd_build_cmd = self.cd_to_build()
        clean_cmd = "make clean"
        retcodes = ClusterRun(self.config, ClusterRunConcatCommands([cd_code_cmd, cd_build_cmd, clean_cmd]))
        return ClusterRunSummaryCode(retcodes)

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
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['raknet', 'sst', 'sirikata']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            retval = cluster_build.dependencies(deps)
        elif cmd == 'update_dependencies':
            deps = []
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['raknet', 'sst', 'sirikata']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            retval = cluster_build.update_dependencies(deps)
        elif cmd == 'patch_build_sst':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            retval = cluster_build.patch_build_sst(patch_file)
        elif cmd == 'build':
            retval = cluster_build.build()
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
        else:
            print "Unknown command: ", cmd
            exit(-1)

        if (retval != 0):
            print "Error while running command '", cmd, "', exiting"
            exit(-1)
