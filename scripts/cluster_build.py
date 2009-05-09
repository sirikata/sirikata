#!/usr/bin/python

from cluster_config import ClusterConfig
from cluster_run import ClusterRun

class ClusterBuild:
    def __init__(self, config):
        self.config = config

    def cd_to_code(self):
        return "cd " + self.config.code_dir + "; "

    def cd_to_build(self):
        return "cd build/cmake; "

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

    def dependencies(self):
        cd_cmd = self.cd_to_code()
        build_cmd = "./install-deps.sh"
        ClusterRun(self.config, cd_cmd + build_cmd)

    def update_dependencies(self):
        cd_cmd = self.cd_to_code()
        update_cmd = "./install-deps.sh update"
        ClusterRun(self.config, cd_cmd + update_cmd)

    def build(self):
        cd_cmd = self.cd_to_code() + self.cd_to_build()
        build_cmd = "cmake .; make -j2"
        ClusterRun(self.config, cd_cmd + build_cmd)

if __name__ == "__main__":
    cc = ClusterConfig()
    cluster_build = ClusterBuild(cc)
    cluster_build.destroy()
    cluster_build.checkout()
    cluster_build.update()
    cluster_build.dependencies()
    cluster_build.update_dependencies()
    cluster_build.build()
