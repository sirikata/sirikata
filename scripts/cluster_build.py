#!/usr/bin/python

import sys
from cluster_config import ClusterConfig
from cluster_run import ClusterRun
from cluster_scp import ClusterSCP

class ClusterBuild:
    def __init__(self, config):
        self.config = config

    def cd_to_code(self):
        return "cd " + self.config.code_dir + "; "

    def cd_to_build(self):
        return "cd build/cmake; "

    def cd_to_sst_build(self):
        return "cd " + self.config.code_dir + "; cd dependencies/sst;"

    def destroy(self):
        destroy_cmd = "rm -rf " + self.config.code_dir + ";"
        ClusterRun(self.config, destroy_cmd)

    def checkout(self):
        checkout_cmd = "git clone git@ahoy:cbr.git " + self.config.code_dir + ";"
        ClusterRun(self.config, checkout_cmd)

    def update(self):
        cd_cmd = self.cd_to_code()
        pull_cmd = "git pull origin;"
        ClusterRun(self.config, cd_cmd + pull_cmd)

    def apply_patch(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "patch -p1 < " + patch_file + ";"
        ClusterRun(self.config, cd_cmd + patch_cmd)

    def apply_patch_mail(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/"+patch_file])
        cd_cmd = self.cd_to_code()
        patch_cmd = "git am " + patch_file + ";"
        ClusterRun(self.config, cd_cmd + patch_cmd)

    def reset_to_head(self):
        cd_cmd = self.cd_to_code()
        reset_cmd = "git reset --hard HEAD;"
        ClusterRun(self.config, cd_cmd + reset_cmd);

    def reset_to_origin_head(self):
        cd_cmd = self.cd_to_code()
        reset_cmd = "git reset --hard origin/HEAD;"
        ClusterRun(self.config, cd_cmd + reset_cmd);

    def dependencies(self, which = None):
        cd_cmd = self.cd_to_code()
        build_cmd = "./install-deps.sh"
        if which != None:
            for dep in which:
                build_cmd += " " + dep
        ClusterRun(self.config, cd_cmd + build_cmd)

    def update_dependencies(self, which = None):
        cd_cmd = self.cd_to_code()
        update_cmd = "./install-deps.sh update"
        if which != None:
            for dep in which:
                update_cmd += " " + dep
        ClusterRun(self.config, cd_cmd + update_cmd)

    def patch_build_sst(self, patch_file):
        ClusterSCP(self.config, [patch_file, "remote:"+self.config.code_dir+"/dependencies/sst/"+patch_file])
        cd_cmd = self.cd_to_sst_build()
        reset_cmd = "git reset --hard HEAD;"
        patch_cmd = "patch -p1 < " + patch_file + ";"
        build_cmd = "make; make install;"
        ClusterRun(self.config, cd_cmd + reset_cmd + patch_cmd + build_cmd)
        # doing this implies we need to rebuild cbr
        self.clean()
        self.build()

    def build(self):
        cd_cmd = self.cd_to_code() + self.cd_to_build()
        build_cmd = "cmake .; make -j2"
        ClusterRun(self.config, cd_cmd + build_cmd)

    def clean(self):
        cd_cmd = self.cd_to_code() + self.cd_to_build()
        clean_cmd = "make clean;"
        ClusterRun(self.config, cd_cmd + clean_cmd)

    def clean_git(self):
        cd_cmd = self.cd_to_code()
        clean_cmd = "git clean -f;"
        ClusterRun(self.config, cd_cmd + clean_cmd)


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

        if cmd == 'destroy':
            cluster_build.destroy()
        elif cmd == 'checkout':
            cluster_build.checkout()
        elif cmd == 'update':
            cluster_build.update()
        elif cmd == 'dependencies':
            deps = []
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['raknet', 'sst', 'sirikata']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            cluster_build.dependencies(deps)
        elif cmd == 'update_dependencies':
            deps = []
            while cur_arg_idx < len(sys.argv) and sys.argv[cur_arg_idx] in ['raknet', 'sst', 'sirikata']:
                deps.append(sys.argv[cur_arg_idx])
                cur_arg_idx += 1
            cluster_build.update_dependencies(deps)
        elif cmd == 'patch_build_sst':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            cluster_build.patch_build_sst(patch_file)
        elif cmd == 'build':
            cluster_build.build()
        elif cmd == 'patch':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            cluster_build.apply_patch(patch_file)
        elif cmd == 'patchmail':
            patch_file = sys.argv[cur_arg_idx]
            cur_arg_idx += 1
            cluster_build.apply_patch_mail(patch_file)
        elif cmd == 'reset':
            cluster_build.reset_to_head()
        elif cmd == 'reset_origin':
            cluster_build.reset_to_origin_head()
        elif cmd == 'clean':
            cluster_build.clean()
        elif cmd == 'clean_git':
            cluster_build.clean_git()
        elif cmd == 'fullbuild':
            cluster_build.destroy()
            cluster_build.checkout()
            cluster_build.update()
            cluster_build.dependencies()
            cluster_build.update_dependencies()
            cluster_build.build()
