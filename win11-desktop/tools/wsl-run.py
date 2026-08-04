#!/usr/bin/env python3
# =====================================================================
#  WSL 命令运行器 (Python 版本)
#  用法: python wsl-run.py [选项] <命令> [参数...]
#  放置于: D:\MyOS\win11-desktop\tools\wsl-run.py
# =====================================================================

import subprocess
import sys
import os
import argparse
import shlex

def main():
    parser = argparse.ArgumentParser(
        description='WSL 命令运行器 - 在 Windows 中运行 Linux 命令',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python wsl-run.py ls -la
  python wsl-run.py -d Ubuntu-20.04 python3 --version
  python wsl-run.py -u root apt update
  python wsl-run.py -w /home/user pwd
  python wsl-run.py -o output.txt "echo 'Hello'"
  python wsl-run.py -q "command"
        """
    )
    
    parser.add_argument('command', nargs='+', help='要执行的命令')
    parser.add_argument('-d', '--distro', help='指定 WSL 分发版')
    parser.add_argument('-u', '--user', help='指定用户')
    parser.add_argument('-w', '--workdir', help='指定工作目录')
    parser.add_argument('-o', '--output', help='保存输出到文件')
    parser.add_argument('-q', '--quiet', action='store_true', help='安静模式')
    parser.add_argument('-v', '--verbose', action='store_true', help='显示详细信息')
    parser.add_argument('-s', '--show-exit', action='store_true', help='显示退出码')
    parser.add_argument('--raw-output', action='store_true', help='原始输出')
    
    args = parser.parse_args()
    
    # 检查 WSL
    try:
        subprocess.run(['wsl', '--version'], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("[ERROR] WSL 未安装或不在 PATH 中", file=sys.stderr)
        print("请先安装 WSL: wsl --install", file=sys.stderr)
        return 1
    
    # 构建命令
    wsl_cmd = ['wsl']
    
    if args.distro:
        wsl_cmd.extend(['--distribution', args.distro])
    
    if args.user:
        wsl_cmd.extend(['--user', args.user])
    
    if args.workdir:
        wsl_cmd.extend(['--cd', args.workdir])
    
    # 通过 bash -c 执行 (正确处理带空格/管道的命令)
    wsl_cmd.extend(['-e', 'bash', '-c'])
    
    # 添加命令
    cmd_str = ' '.join(args.command)
    wsl_cmd.append(cmd_str)
    
    # 显示执行的命令
    if args.verbose:
        print(f"[INFO] 执行: {' '.join(wsl_cmd)}")
        print()
    
    # 执行命令
    try:
        if args.quiet:
            # 安静模式
            result = subprocess.run(wsl_cmd, capture_output=True, text=True)
        elif args.output:
            # 输出到文件
            with open(args.output, 'w', encoding='utf-8') as f:
                result = subprocess.run(wsl_cmd, stdout=f, stderr=subprocess.STDOUT, text=True)
            if not args.quiet:
                print(f"[INFO] 输出保存到: {args.output}")
        else:
            # 默认模式
            result = subprocess.run(wsl_cmd, capture_output=True, text=True)
            
            # 显示输出
            if args.raw_output:
                sys.stdout.write(result.stdout)
                sys.stderr.write(result.stderr)
            else:
                # 带颜色输出
                for line in result.stdout.splitlines():
                    if 'ERROR' in line:
                        print(f"\033[91m{line}\033[0m")
                    elif 'WARNING' in line:
                        print(f"\033[93m{line}\033[0m")
                    elif 'INFO' in line:
                        print(f"\033[96m{line}\033[0m")
                    elif 'SUCCESS' in line:
                        print(f"\033[92m{line}\033[0m")
                    else:
                        print(line)
                
                if result.stderr:
                    for line in result.stderr.splitlines():
                        print(f"\033[91m{line}\033[0m", file=sys.stderr)
        
        exit_code = result.returncode
        
    except Exception as e:
        print(f"[ERROR] 执行失败: {e}", file=sys.stderr)
        return 1
    
    # 显示退出码
    if args.show_exit and not args.quiet:
        print()
        print(f"[INFO] 退出码: {exit_code}")
    
    return exit_code

if __name__ == '__main__':
    sys.exit(main())
