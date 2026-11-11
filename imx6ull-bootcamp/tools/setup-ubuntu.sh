#!/usr/bin/env bash
#
# setup-ubuntu.sh - one-shot build-host setup for the i.MX6ULL board.
#
# RUN THIS INSIDE THE UBUNTU VM (VMware, 22.04), not on Windows, not on the board.
#
#     chmod +x setup-ubuntu.sh
#     ./setup-ubuntu.sh
#
# It does five things and is safe to re-run:
#   1. installs the cross toolchain, tftp, NFS server, device-tree tools
#   2. creates the standard ~/imx6ull/{toolchain,uboot,kernel,rootfs,tftpboot,nfsroot,module,app}
#   3. points tftpd-hpa at ~/imx6ull/tftpboot
#   4. exports ~/imx6ull/nfsroot over NFS with no_root_squash
#   5. prints the exact commands to run on the board to verify
#
# Why these two services: tftp is how U-Boot fetches zImage/dtb over the network,
# and NFS is how the board runs binaries straight out of your Ubuntu directory,
# so you never scp after every edit.

set -euo pipefail

if [[ ${EUID} -eq 0 ]]; then
    echo "Run this as your normal user - it calls sudo itself." >&2
    exit 1
fi

if ! command -v sudo >/dev/null 2>&1; then
    echo "sudo is required but not installed." >&2
    exit 1
fi

if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WARNING: this looks like WSL. Bridged networking and NFS are painful" >&2
    echo "         in WSL; VMware + Ubuntu 22.04 is strongly recommended." >&2
    echo "         Continuing anyway..." >&2
fi

BASE="$HOME/imx6ull"

echo "==> [1/5] Installing packages (needs internet, takes a few minutes)"
sudo apt-get update
sudo apt-get install -y \
    build-essential git vim curl wget rsync unzip \
    gcc-arm-linux-gnueabihf \
    tftp-hpa tftpd-hpa nfs-kernel-server \
    minicom picocom device-tree-compiler \
    bc bison flex libssl-dev libncurses-dev \
    u-boot-tools net-tools openssh-server

echo "==> [2/5] Creating directory layout under $BASE"
mkdir -p "$BASE"/{toolchain,uboot,kernel,rootfs,tftpboot,nfsroot,module,app}
ls -1 "$BASE"

echo "==> [3/5] Configuring tftpd-hpa to serve $BASE/tftpboot"
sudo tee /etc/default/tftpd-hpa >/dev/null <<EOF
TFTP_USERNAME="tftp"
TFTP_DIRECTORY="$BASE/tftpboot"
TFTP_ADDRESS="0.0.0.0:69"
TFTP_OPTIONS="--secure --create"
EOF
sudo chown -R tftp:tftp "$BASE/tftpboot"
sudo systemctl restart tftpd-hpa

echo "==> [4/5] Exporting $BASE/nfsroot over NFS"
if grep -qF "$BASE/nfsroot" /etc/exports 2>/dev/null; then
    echo "    already in /etc/exports, skipping"
else
    echo "$BASE/nfsroot *(rw,sync,no_subtree_check,no_root_squash,vers=3)" \
        | sudo tee -a /etc/exports >/dev/null
fi
sudo exportfs -arv
sudo systemctl restart nfs-kernel-server

echo "==> [5/5] Done"
IP=$(ip -4 -o addr show scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -1 || true)

cat <<EOF

  ------------------------------------------------------------------
  Ubuntu IP (use this as U-Boot's serverip): ${IP:-<run: ip -br addr>}
  Toolchain: $(arm-linux-gnueabihf-gcc --version 2>/dev/null | head -1 || echo 'not found')

  On the board (plug the cable into ETH1 or ETH2):
      cat /sys/class/net/eth1/carrier     # 1 = cable is in this port
      sudo dhclient eth1 || sudo udhcpc -i eth1
      ip -br addr
      ping -c 3 ${IP:-<ubuntu-ip>}

  Mount your apps over NFS (edit code on Ubuntu, run on the board):
      sudo mount -t nfs -o nolock,vers=3 ${IP:-<ubuntu-ip>}:$BASE/nfsroot /mnt

  Then go to imx6ull-bootcamp/m0-hello and run: make check
  ------------------------------------------------------------------

EOF
