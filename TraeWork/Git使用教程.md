# Git 使用教程（单人管理）

> 面向个人开发者，覆盖日常最常用的 Git 操作。不涉及多人协作（分支合并、PR、Code Review 等）。

---

## 目录

1. [Git 是什么](#1-git-是什么)
2. [安装 Git](#2-安装-git)
3. [首次配置](#3-首次配置)
4. [创建仓库](#4-创建仓库)
5. [日常开发流程（核心）](#5-日常开发流程核心)
6. [查看状态和历史](#6-查看状态和历史)
7. [撤销与回退](#7-撤销与回退)
8. [远程仓库（GitHub）](#8-远程仓库github)
9. [.gitignore 忽略文件](#9-gitignore-忽略文件)
10. [常用命令速查](#10-常用命令速查)

---

## 1. Git 是什么

Git 是一个**版本控制系统**。简单说，它帮你：

- **记录每次修改**：像游戏存档一样，随时可以回到之前的版本
- **对比差异**：看看这次改了什么
- **同步到远程**：备份到 GitHub，换电脑也能继续开发

**三个核心概念：**

```
工作目录（你写的代码）
    │  git add
    ▼
暂存区（准备提交的修改）
    │  git commit
    ▼
本地仓库（已保存的版本）
    │  git push
    ▼
远程仓库（GitHub 上的备份）
```

---

## 2. 安装 Git

**Windows：** 去 [git-scm.com](https://git-scm.com/download/win) 下载安装包，一路默认安装即可。

安装完成后，在任意文件夹右键 → "Open Git Bash here"（或用 PowerShell/CMD），输入以下命令验证：

```bash
git --version
# 输出示例：git version 2.45.0.windows.1
```

---

## 3. 首次配置

安装后需要配置用户名和邮箱（提交记录会带上这个信息）：

```bash
git config --global user.name "你的名字"
git config --global user.email "你的邮箱@example.com"
```

> 如果只对当前项目设置（去掉 `--global`）：
> ```bash
> git config user.name "TuesMagique-Bo"
> git config user.email "xxx@github.com"
> ```

查看配置是否生效：

```bash
git config --list
```

---

## 4. 创建仓库

### 4.1 从头开始（本地已有代码）

```bash
cd 你的项目文件夹
git init                    # 初始化，创建 .git 隐藏文件夹
git add -A                  # 添加所有文件到暂存区
git commit -m "初始提交"     # 提交到本地仓库
```

### 4.2 从 GitHub 克隆（远程已有代码）

```bash
git clone https://github.com/用户名/仓库名.git
cd 仓库名
```

---

## 5. 日常开发流程（核心）

这是你每天都会用到的操作流程：

```bash
# 1. 写代码...改了一堆文件

# 2. 查看改了哪些文件
git status

# 3. 查看具体改了什么内容
git diff

# 4. 添加到暂存区
git add 文件名          # 添加指定文件
git add src/main.c     # 示例：只添加 main.c
git add -A             # 添加所有修改

# 5. 提交到本地仓库
git commit -m "修复了PWM占空比计算错误"

# 6. 推送到 GitHub（如果有远程仓库）
git push
```

### 5.1 提交信息怎么写

提交信息应该简洁说明**做了什么**、**为什么**：

| 推荐 | 不推荐 |
|------|--------|
| `修复电机PWM频率不匹配问题` | `修改` |
| `新增MPU9250初始化代码` | `更新` |
| `添加手动模式切换命令` | `123` |
| `调整DLPF带宽为41Hz降低噪声` | `改了点东西` |

### 5.2 完整示例

```bash
# 修改了 comm.c 和 motor.c
git status
# 输出：
#   modified:   Modules/Comm/Src/comm.c
#   modified:   Modules/Motor/Src/motor.c

git add Modules/Comm/Src/comm.c Modules/Motor/Src/motor.c
# 或者直接：git add -A

git commit -m "新增MODE MANUAL命令，手动模式下Control_Update跳过PID"

git push
```

---

## 6. 查看状态和历史

```bash
# 查看当前状态（哪些文件改了、哪些已暂存）
git status

# 查看简要状态
git status -s

# 查看提交历史（最近 N 条）
git log --oneline -10

# 查看某次提交的详细内容
git show 提交ID

# 查看某次提交改了哪些文件
git show --stat 提交ID

# 查看某个文件的修改历史
git log --oneline 文件名
```

---

## 7. 撤销与回退

### 7.1 还没 git add（撤销工作区修改）

```bash
# 撤销某个文件的修改，回到上次提交的状态
git checkout -- 文件名

# 撤销所有修改
git checkout -- .
```

### 7.2 已经 git add 但还没 git commit（撤销暂存）

```bash
# 把某个文件从暂存区移回工作区（修改还在，只是不提交了）
git reset HEAD 文件名

# 撤销所有暂存
git reset HEAD
```

### 7.3 已经 git commit 但还没 git push（撤销提交）

```bash
# 撤销最近一次提交，修改保留在工作区
git reset --soft HEAD~1

# 撤销最近一次提交，修改保留但取消暂存
git reset --mixed HEAD~1

# 撤销最近一次提交，修改全部丢弃（危险！）
git reset --hard HEAD~1
```

### 7.4 已经 git push（回退远程版本）

```bash
# 1. 本地回退到指定版本
git reset --hard 提交ID

# 2. 强制推送（覆盖远程）
git push --force
```

> 强制推送会覆盖远程仓库内容，仅限个人仓库使用。

---

## 8. 远程仓库（GitHub）

### 8.1 关联远程仓库

```bash
# 添加远程仓库
git remote add origin https://github.com/用户名/仓库名.git

# 查看已关联的远程仓库
git remote -v
```

### 8.2 推送代码

```bash
# 首次推送（设置上游分支）
git push -u origin main

# 后续推送
git push
```

### 8.3 拉取代码（换电脑时）

```bash
# 从远程拉取最新代码
git pull

# 或者分两步：
git fetch           # 拉取远程更新
git merge           # 合并到本地
```

### 8.4 完整流程：从零到 GitHub

```bash
# 1. GitHub 上创建空仓库（不要勾选 README/.gitignore）

# 2. 本地初始化并推送
cd 你的项目
git init
git add -A
git commit -m "初始提交"
git remote add origin https://github.com/用户名/仓库名.git
git branch -M main
git push -u origin main
```

---

## 9. .gitignore 忽略文件

有些文件不应该提交到仓库（编译产物、临时文件、密钥等），在项目根目录创建 `.gitignore`：

```gitignore
# 编译产物
*.o
*.elf
*.hex
*.bin
build/
Debug/

# IDE 配置
.vscode/
.idea/

# 系统文件
Thumbs.db
.DS_Store
Desktop.ini

# 密钥和配置
.env
*.key
```

**常用通配符：**

| 写法 | 含义 |
|------|------|
| `*.o` | 忽略所有 .o 文件 |
| `build/` | 忽略 build 文件夹 |
| `!important.o` | 不忽略 important.o（例外） |
| `**/log/` | 忽略任意层级的 log 文件夹 |

---

## 10. 常用命令速查

```bash
# ─── 初始化 ───
git init                              # 初始化仓库
git clone <url>                       # 克隆远程仓库

# ─── 日常操作 ───
git status                            # 查看状态
git diff                              # 查看修改内容
git add <文件>                        # 添加到暂存区
git add -A                            # 添加所有修改
git commit -m "说明"                  # 提交
git push                              # 推送到远程
git pull                              # 拉取远程更新

# ─── 查看历史 ───
git log --oneline -10                 # 最近 10 条提交
git log --oneline --graph             # 图形化分支历史
git show <提交ID>                     # 查看某次提交详情

# ─── 撤销 ───
git checkout -- <文件>                # 撤销工作区修改
git reset HEAD <文件>                 # 取消暂存
git reset --soft HEAD~1               # 撤销最近提交（保留修改）
git reset --hard HEAD~1               # 撤销最近提交（丢弃修改）

# ─── 远程仓库 ───
git remote -v                         # 查看远程仓库
git remote add origin <url>           # 添加远程仓库
git push -u origin main               # 首次推送
git push --force                      # 强制推送（个人仓库用）

# ─── 标签（标记版本） ───
git tag v1.0                          # 打标签
git tag -a v1.0 -m "第一版"           # 带说明的标签
git push --tags                       # 推送标签到远程
git tag                               # 查看所有标签
```

---

## 附录：典型工作场景

### 场景 1：每天开始写代码

```bash
git pull            # 拉取最新代码（如果有远程）
# 开始写代码...
```

### 场景 2：写完一个功能，想保存

```bash
git add -A
git commit -m "实现了XXX功能"
git push
```

### 场景 3：改坏了，想回到之前的版本

```bash
git log --oneline       # 找到想回退的版本ID
git reset --hard 版本ID  # 回退
```

### 场景 4：想暂时保存当前进度，切换到其他任务

```bash
git stash               # 暂存当前修改
# 干别的事...
git stash pop           # 恢复之前的修改
```

### 场景 5：误删了文件，想恢复

```bash
git checkout -- 被删的文件名
```

### 场景 6：想看看某个文件在之前版本长什么样

```bash
git log --oneline 文件名          # 找到相关提交
git show 提交ID:文件路径           # 查看那个版本的文件内容
```

---

> 更多内容参考：[Git 官方文档](https://git-scm.com/doc) 或 `git --help`