module Ray
  class Vertex
    def x; pos.x; end
    def y; pos.y; end

    def x=(val); self.pos = [val, y]; end
    def y=(val); self.pos = [x, val]; end

    def tex_x; tex.x; end
    def tex_y; tex.y; end

    def tex_x=(val); self.tex = [val, tex_y]; end
    def tex_y=(val); self.tex = [tex_x, val]; end

    alias color  col
    alias color= col=
  end
end
