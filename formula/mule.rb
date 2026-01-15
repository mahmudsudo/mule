class Mule < Formula
  desc "Minimalist C++ Build System & Package Manager"
  homepage "https://github.com/mahmudsudo/mule"
  url "https://github.com/mahmudsudo/mule/archive/refs/tags/v0.1.0.tar.gz"
  sha256 "69d52f9f00607b067cc9d14e320c825de61ba9d2ae801614e039ed110815d993"
  license "MIT"

  depends_on "cmake" => :build # Mule can use cmake dependencies, good to have.
  # depends_on "gcc" # specific compiler needs?

  def install
    # Build from source
    system "g++", "-std=c++17", "src/main.cpp", *Dir["src/core/*.cpp"], "-o", "mule"
    bin.install "mule"
  end

  test do
    system "#{bin}/mule", "--help"
  end
end
