#!/bin/bash

# TinyRPC 构建脚本
# 用法: ./scripts/build.sh [选项]
# 选项:
#   --clean     清理构建目录
#   --release   发布版本构建
#   --debug     调试版本构建
#   --test      运行测试
#   --install   安装到系统

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 默认参数
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=false
INSTALL=false
JOBS=$(nproc)

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo "选项:"
            echo "  --clean     清理构建目录"
            echo "  --release   发布版本构建 (默认)"
            echo "  --debug     调试版本构建"
            echo "  --test      运行测试"
            echo "  --install   安装到系统"
            echo "  -j N        使用N个并行作业"
            echo "  -h, --help  显示此帮助信息"
            exit 0
            ;;
        *)
            print_error "未知选项: $1"
            exit 1
            ;;
    esac
done

# 进入项目根目录
cd "$PROJECT_ROOT"

print_info "TinyRPC 构建脚本"
print_info "项目根目录: $PROJECT_ROOT"
print_info "构建类型: $BUILD_TYPE"
print_info "并行作业数: $JOBS"

# 检查依赖
print_info "检查构建依赖..."

check_command() {
    if ! command -v $1 &> /dev/null; then
        print_error "$1 未找到，请先安装"
        exit 1
    fi
}

check_command cmake
check_command make
check_command g++

# 检查依赖库
print_info "检查依赖库..."

check_library() {
    if ! pkg-config --exists $1 2>/dev/null; then
        print_warning "$1 库未找到，可能需要手动安装"
    else
        print_success "$1 库已找到"
    fi
}

check_library protobuf

# 清理构建目录
if [ "$CLEAN_BUILD" = true ]; then
    print_info "清理构建目录..."
    rm -rf build
    rm -rf bin
    rm -rf lib
    print_success "构建目录已清理"
fi

# 创建构建目录
print_info "创建构建目录..."
mkdir -p build
mkdir -p bin
mkdir -p lib
mkdir -p logs

# 进入构建目录
cd build

# 配置构建
print_info "配置构建..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local

if [ $? -ne 0 ]; then
    print_error "CMake 配置失败"
    exit 1
fi

print_success "CMake 配置完成"

# 编译
print_info "开始编译..."
make -j$JOBS

if [ $? -ne 0 ]; then
    print_error "编译失败"
    exit 1
fi

print_success "编译完成"

# 运行测试
if [ "$RUN_TESTS" = true ]; then
    print_info "运行测试..."
    
    # 检查测试可执行文件是否存在
    if [ -f "../bin/tinyrpc_test" ]; then
        cd ..
        ./bin/tinyrpc_test
        if [ $? -eq 0 ]; then
            print_success "所有测试通过"
        else
            print_error "测试失败"
            exit 1
        fi
        cd build
    else
        print_warning "测试可执行文件未找到，跳过测试"
    fi
fi

# 安装
if [ "$INSTALL" = true ]; then
    print_info "安装到系统..."
    sudo make install
    
    if [ $? -eq 0 ]; then
        print_success "安装完成"
    else
        print_error "安装失败"
        exit 1
    fi
fi

# 显示构建结果
print_success "构建完成！"
echo
print_info "可执行文件位置:"
ls -la ../bin/ 2>/dev/null || print_warning "bin目录为空"

print_info "库文件位置:"
ls -la ../lib/ 2>/dev/null || print_warning "lib目录为空"

echo
print_info "运行示例:"
echo "  服务端: ./bin/userservice_server"
echo "  客户端: ./bin/userservice_client"
echo "  基准测试: ./bin/tinyrpc_benchmark --threads=4 --duration=60s"

print_success "构建脚本执行完成！"