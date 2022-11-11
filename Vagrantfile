######################################################################
#
#                       Author: Pranav Panganamamula
#                       Date:   08/29/2021
#
######################################################################

## -*- mode: ruby -*-
## vi: set ft=ruby :

# The "2" in Vagrant.configure configures the configuration version.
# Please don't change it unless you know what you're doing.
Vagrant.configure(2) do |config|
  ubuntu_name = "focal"
  config.vm.box = "ubuntu/focal64"
  config.vm.box_check_update = false

  config.vm.hostname = "cis380Dev"

  ## Provisioning
  config.vm.provision "shell", inline: <<-SHELL
      llvm_url=apt.llvm.org
      llvm_version=10
  
      wget -O - https://$llvm_url/llvm-snapshot.gpg.key | apt-key add -
      add-apt-repository "deb http://$llvm_url/#{ubuntu_name}/ \
                              llvm-toolchain-#{ubuntu_name}-$llvm_version main"
  
      apt update
      apt upgrade -y
      apt install -y clang-$llvm_version emacs gdb make valgrind
  
      cd $(dirname $(which clang-$llvm_version)) && \
      ln -s clang-$llvm_version clang
  
      echo -e '\nPATH=.:$PATH\ncd /vagrant' >> /home/vagrant/.profile
  
      reboot
  SHELL

  ## CPU & RAM
  config.vm.provider "virtualbox" do |vb|
      # By pass a known bug by setting file to serial port
      # https://bugs.launchpad.net/cloud-images/+bug/1829625
      vb.customize ["modifyvm", :id, "--uart1", "0x3F8", "4"]
      vb.customize ["modifyvm", :id, "--uartmode1", "file", File::NULL]
      # Setup memory and cups
      vb.customize ["modifyvm", :id, "--cpuexecutioncap", "100"]
      vb.memory = 2048
      vb.cpus = 2
  end
  config.vagrant.plugins = "vagrant-vbguest"
  end