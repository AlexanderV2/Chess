#include "piece.h"
#include "freepieces.h"
#include "piecemove.h"


LPiece::LPiece(LPiece::Type pieceType,
               bool white,
               LGrid *initLGrid,
               LBoard *l_board)
    : type(pieceType), isWhite(white), lgrid(initLGrid), lboard(l_board)
{}

LPiece::~LPiece()
{}


// Check if move valid and not brings king in danger
bool LPiece::isMoveValid(LGrid *grid_moveTo)
{
    bool isMoveValid {true};

    this->makeMove(grid_moveTo, true);

    if ( lboard->isCheck(isWhite) ) {
        isMoveValid = false;
    }

    lboard->undoMove();
    return isMoveValid;
}


void LPiece::remove()
{
    this->inGame = false;
    this->lgrid->lpiece = nullptr;
    this->lgrid = nullptr;
}

void LPiece::placeTo(LGrid *lgrid_to)
{
#ifdef _DEBUG
    assert(lgrid_to != nullptr);
#endif

    if (this->lgrid != nullptr){
        this->lgrid->lpiece = nullptr;
    }

    this->lgrid = lgrid_to;
    this->lgrid->lpiece = this;
}


void LPiece::clearMoves()
{
    moves = 0;
}


// TODO: check
bool LPiece::isProtected() const
{
    return this->lgrid->is_attacked(isWhite);
}


void LPiece::makeMove(LGrid *gridTo, bool fake)
{

    if( lboard->move != isWhite ||
            (!gridTo->empty() && gridTo->lpiece->isWhite == isWhite) )
    {
#ifdef _DEBUG
        qDebug() << "makeMove ERROR: wrong move";
#endif
        return;
    }

    lboard->currentMove = new PieceMove(this, lgrid, gridTo);
    lboard->currentMove->fake = fake;

    // Castling
    if (type == King) {
        if (lgrid->offset(2, 0) == lboard->currentMove->to) {
            LPiece *Rook = lgrid->offset(3, 0)->lpiece;
            Rook->placeTo(Rook->lgrid->offset(-2, 0));
            lboard->currentMove->extra = true;
        } else if (lgrid->offset(-2, 0) == lboard->currentMove->to) {
            LPiece *Rook = lgrid->offset(-4, 0)->lpiece;
            Rook->placeTo(Rook->lgrid->offset(3, 0));
            lboard->currentMove->extra = true;
        }
    }

    LPiece *pieceRemoved = nullptr;

    //Pawn's extra move
    if (type == Pawn) {
        LGrid *g, *g2;
        LPiece *f;
        int up = isWhite ? 1 : -1,
            v[] = {1, -1};

        for(int i=0; i<2; i++)

        if ( (g=lgrid->offset(v[i], up)) &&
             g->empty() &&
             g == lboard->currentMove->to &&
             !(g2=g->offset(0, -up))->empty() &&
             (f=g2->lpiece)->type == Pawn &&
              f->isWhite != isWhite
              )
        {
            (pieceRemoved = f)->remove();
            lboard->currentMove->extra = true;
            break;
        }
    }

    if (!lboard->currentMove->to->empty()) {
        (pieceRemoved = lboard->currentMove->to->lpiece)->remove();
    }

    if (pieceRemoved) {
        lboard->currentMove->removed = pieceRemoved;
    }

    this->placeTo(lboard->currentMove->to);

    if (type == Pawn && lgrid->y == (isWhite ? 7 : 0)) {
        type = Queen;
        lboard->currentMove->extra = true;
    }

    lboard->moves->push_back(lboard->currentMove);
    this->moves++;

    if (!fake) {
        lboard->positionChanged = true;
    }

    lboard->move = !lboard->move;
}


bool LPiece::isMoved() const
{
    return moves > 0;
}

QVector<LGrid *> LPiece::getGrids(bool getAttacked)
{
    if (lboard->positionChanged) {
        gridsCached = false;
        attackedGridsCached = false;
    } else if (lboard->moves->size() && !lboard->moves->back()->fake) {
        // Check & return if cached, if last move is not fake
        if (getAttacked && attackedGridsCached) {
            return m_cachedAttackedGrids;
        } else if (!getAttacked && gridsCached) {
            return m_cachedGrids;
        }
    }

    QVector<LGrid *> moves;

    if (pieceBehavior[type].extra) {

        if (type == Pawn) {

            LGrid* g;
            int up = isWhite ? 1 : -1;
            if ( !getAttacked ) {

                if ( (g=lgrid->offset(0, up)) !=nullptr && g->empty()
                     && isMoveValid(g))
                {
                    moves.push_back(g);
                }

                // first pos
                if ( lgrid->y == (isWhite?1:6) &&
                     lgrid->offset(0, up)->empty() &&
                     (g=lgrid->offset(0, up*2))->empty() && isMoveValid(g) )
                {
                    moves.push_back(g);
                }
            }

            // *** //

            if((g=lgrid->offset( 1, up)) && (getAttacked || (!g->empty() && g->lpiece->isWhite != isWhite && isMoveValid(g)) ))
                moves.push_back(g);

            if((g=lgrid->offset(-1, up)) && (getAttacked || (!g->empty() && g->lpiece->isWhite != isWhite && isMoveValid(g)) ))
                moves.push_back(g);



            if ( (g=lgrid->offset(1, 0)) && // grid exists
                !g->empty() && // and not empty
                g->lpiece->type == Pawn && // and grid's piece is Pawn
                g->lpiece->isWhite != isWhite && // enemie's Pawn
                g->lpiece->moves == 1 && // that moved only one time
                lboard->moves->back()->lpiece == g->lpiece && // and that move is last move
                g->lpiece->lgrid->y == (isWhite ? 4 : 3) && // it was long move
                (g = g->offset(0, up)) && // move is grid to up
                g->empty() && // if this grid empty
                isMoveValid(g))
            {
                moves.push_back(g);
            }


            if ( (g=lgrid->offset(-1, 0)) &&
                !g->empty() &&
                g->lpiece->type == Pawn &&
                g->lpiece->isWhite != isWhite &&
                g->lpiece->moves == 1 &&
                lboard->moves->back()->lpiece == g->lpiece  &&
                g->lpiece->lgrid->y == (isWhite ? 4 : 3) &&
                (g = g->offset(0, up)) &&
                g->empty() &&
                isMoveValid(g))
            {
                moves.push_back(g);
            }

        }

        else if(type == King)
        {
            const int mv[8][2] = {
              { 1,0}, { 1, 1}, {0, 1}, {-1, 1},
              {-1,0}, {-1,-1}, {0,-1}, { 1,-1}
            };

            for(int i=0; i<8; i++) {
                LGrid* p_grid = lgrid->offset(mv[i][0], mv[i][1]);
                if (!p_grid) {
                    continue;
                }

                if (getAttacked) {
                    moves.push_back(p_grid);
                    continue;
                }

                if ( p_grid->empty() && (!getAttacked && !p_grid->is_attacked(!isWhite) && isMoveValid(p_grid))) {
                    moves.push_back(p_grid);
                }

                else if ( !p_grid->empty() &&
                          p_grid->lpiece->isWhite != isWhite &&
                          !p_grid->lpiece->isProtected() &&
                          isMoveValid(p_grid)
                          )
                {
                    moves.push_back(p_grid);
                }
            }

             // Castling
            if ( !getAttacked && !isMoved() && !lboard->isCheck(isWhite) ) {
                LPiece *Rook;
                LGrid *grd;
                const int y = lgrid->y;

                if ( !lboard->grids[0][y]->empty() &&
                     (Rook = lboard->grids[0][y]->lpiece)->type == LPiece::Rook &&
                     !Rook->isMoved() &&
                     (grd=Rook->lgrid->offset(1, 0))->empty() && !grd->is_attacked(!isWhite) &&
                     (grd=Rook->lgrid->offset(2, 0))->empty() && !grd->is_attacked(!isWhite) &&
                     (grd=Rook->lgrid->offset(3, 0))->empty() && !grd->is_attacked(!isWhite)
                     )
                {
                    moves.push_back(lgrid->offset(-2, 0));
                }

                if ( !lboard->grids[7][y]->empty() &&
                     (Rook = lboard->grids[7][y]->lpiece)->type == LPiece::Rook &&
                     !Rook->isMoved() &&
                     (grd=Rook->lgrid->offset(-1, 0))->empty() && !grd->is_attacked(!isWhite) &&
                     (grd=Rook->lgrid->offset(-2, 0))->empty() && !grd->is_attacked(!isWhite)
                     )
                {
                    moves.push_back(lgrid->offset(2, 0));
                }
            }
        }

        else // Knight
        {
            // How horse moves :)
            const int offsets[8][2] = { {1,2}, {2,1}, {-1,2},  {-2, 1},{-1, -2}, {1, -2},{-2, -1}, {2, -1}};
            for(int i=0; i<8; i++) {
                LGrid* g = lgrid->offset(offsets[i][0], offsets[i][1]);

                if(g && (getAttacked ||
                         ((g->empty() || g->lpiece->isWhite != isWhite) &&
                          isMoveValid(g))) )
                {
                    moves.push_back(g);
                }
           }
        }
    }

    //Generic pieces' move
    else {

        if ( pieceBehavior[type].diagonal ) {


            // right & up
            for(int x = lgrid->x+1, y = lgrid->y+1; x<8 && y < 8; x++, y++) {
                LGrid* off = lgrid->offset(x-lgrid->x, y-lgrid->y);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off) ) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }

            // left && down
            for(int x = lgrid->x-1, y = lgrid->y-1; x>=0 && y >= 0; x--, y--){

                LGrid* off = lgrid->offset(x-lgrid->x, y-lgrid->y);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off)) ){
                        moves.push_back(off);
                    }
                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }

            // up && left
            for(int x = lgrid->x-1, y = lgrid->y+1; x>=0 && y < 8; x--, y++) {

                LGrid* off = lgrid->offset(x-lgrid->x, y-lgrid->y);
                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off)) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }

            for(int x = lgrid->x+1, y = lgrid->y-1; x<8 && y >= 0; x++, y--) {
                LGrid* off = lgrid->offset(x-lgrid->x, y-lgrid->y);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off)) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }


        }


        if ( pieceBehavior[type].straight ) {

            // X

            // right
            for(int i=1; i < 8-lgrid->x; i++){
                LGrid* off = lgrid->offset(i, 0);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off) ) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }


            // left
            for(int i=lgrid->x-1; i>=0; i--){
                LGrid* off = lgrid->offset(i-lgrid->x, 0);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off) ) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }

            // Y

            // top
            for(int i=1; i < 8-lgrid->y; i++)  {
                LGrid* off = lgrid->offset(0, i);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off) )) {
                        moves.push_back(off);
                    }
                    break;
                }

                if( getAttacked || isMoveValid(off)) {
                    moves.push_back(off);
                }
            }

            // bottom
            for(int i=lgrid->y-1; i >= 0; i--) {
                LGrid *off = lgrid->offset(0, i-lgrid->y);

                if(!off->empty()) {
                    if ( getAttacked || (off->lpiece->isWhite != isWhite && isMoveValid(off) ) ){
                        moves.push_back(off);
                    }

                    break;
                }

                if( getAttacked || isMoveValid(off)){
                    moves.push_back(off);
                }
            }
        }
    }

    // Caching moves
    if (lboard->moves->size() && !lboard->moves->back()->fake) {
        if (getAttacked) {
            m_cachedAttackedGrids = moves;
            attackedGridsCached = true;
        } else {
            m_cachedGrids = moves;
            gridsCached = true;
        }
    }

    return moves;
}



Piece::Piece(LPiece *l_piece, Board *_board)
: QGraphicsObject(),  lpiece(l_piece), board(_board)
{
    anim = new QPropertyAnimation(this, "pos");
    setZValue(1);
}

Piece::~Piece()
{
     delete anim;
}

void Piece::placeTo(Grid *grid_to, bool show)
{
    this->lpiece->placeTo(grid_to->lgrid);

    if (this->grid) {
        this->grid->piece = nullptr;
    }

    this->grid = grid_to;
    this->grid->piece = this;

    if (show){
        this->setPos(grid->pos());
    }
}


void Piece::makeMove(Grid *gridTo)
{

    if( board->lboard->move != lpiece->isWhite ||
            (!gridTo->lgrid->empty() &&
             gridTo->lgrid->lpiece->isWhite == lpiece->isWhite) )
    {
#ifdef _DEBUG
        qDebug() << "makeMove ERROR: wrong move" << (board->lboard->move != lpiece->isWhite)
                 <<(!gridTo->lgrid->empty() && gridTo->lgrid->lpiece->isWhite == lpiece->isWhite) ;
#endif
        return;
    }

    if ( board->options->flipBoard ) {
        board->reverse(board->lboard->move);
    }

    board->resetHighligtedGrids();
    this->lpiece->makeMove(gridTo->lgrid);
#ifdef _DEBUG
    assert ( !lpiece->lboard->currentMove->isNull() );
#endif
    board->selected = nullptr;
    board->setPiecesSelectable(false);

    this->setZValue(2);
    this->animateTo(gridTo, true);

    // Castling
    if ( lpiece->lboard->currentMove->extra && lpiece->type == LPiece::King) {
        if (grid->offset(2, 0)->lgrid == lpiece->lboard->currentMove->to) {
            Piece *Rook = grid->offset(3, 0)->piece;
            Grid *g = Rook->grid->offset(-2, 0);
            Rook->animateTo(g);
            Rook->placeTo(g, false);
        } else if (grid->offset(-2, 0)->lgrid == lpiece->lboard->currentMove->to) {
            Piece *Rook = grid->offset(-4, 0)->piece;
            Grid *g = Rook->grid->offset(3, 0);
            Rook->animateTo(g);
            Rook->placeTo(g, false);
        }

    }

    board->timersValue[!board->lboard->move].push(board->timer[!board->lboard->move]->time());
    board->timer[!board->lboard->move]->stop();
    board->timer[board->lboard->move]->start();
}

void Piece::moveEnd()
{
    // this one just fixes SIGSEGV
    if (lpiece->lboard->currentMove == nullptr) {
        return;
    }

    if (lpiece->lboard->currentMove->removed) {
        Piece::get(lpiece->lboard->currentMove->removed, board)->remove();
    }

    this->placeTo(Grid::get(lpiece->lboard->currentMove->to, board));

    board->resetHighligtedGrids();

    // Highlight last move
    Grid::get(lpiece->lboard->currentMove->from, board)->highlight(4);
    Grid::get(lpiece->lboard->currentMove->to, board)->highlight(4);

    board->setPiecesSelectable(true);
    this->setZValue(1);
    anim->disconnect();
    board->pieceMoveCompleted();
}

void Piece::animateTo(Grid *gridTo, bool moveEnd)
{
    if (moveEnd){
        QObject::connect(anim, SIGNAL(finished()), SLOT(moveEnd()));
    }

    anim->setDuration(qMax(600, qMax(qAbs(grid->lgrid->x - gridTo->lgrid->x),
                               qAbs(grid->lgrid->y - gridTo->lgrid->y)) * 200));

    anim->setStartValue(this->grid->pos());
    anim->setEndValue(gridTo->pos());
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->start();

}

void Piece::select()
{
#ifdef _DEBUG
    assert( lpiece->inGame &&
            board->lboard->move == lpiece->isWhite && board->isPiecesSelectable() );
#endif
    board->resetHighligtedGrids();
    QVector<LGrid *> availMoves = lpiece->getGrids();
    board->selected = this;

    grid->highlight();

    if (availMoves.size()) {
        for(int i=0; i<availMoves.size(); i++) {
            Grid::get(availMoves[i], board)->highlight(1);
        }
    }
}

void Piece::remove()
{
    board->free_pieces[lpiece->isWhite]->addPiece(this);
}

Piece *Piece::get(LPiece *lp, Board *board)
{
    if (!lp || !board || lp->index < 0 || lp->index > 31){
#ifdef _DEBUG
        qDebug() << "Piece::get: nullptr returned";
#endif
        return nullptr;
    }

    return board->pieces->at(quint32(lp->index));
}


QRectF Piece::boundingRect() const
{
    return QRectF(0, 0, board->grid_size, board->grid_size);
}

void Piece::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPixmap pieceTile = board->piecesTileset->scaled(board->grid_size * 6, board->grid_size * 2);

    painter->save();
    painter->drawPixmap(0, 0, pieceTile,
                           (lpiece->type-1) * board->grid_size, lpiece->isWhite ? 0 : board->grid_size,
                           board->grid_size*1, board->grid_size);
    painter->restore();
}

bool Piece::isAnimationRunning() const
{
    return this->anim->state() == QAbstractAnimation::Running;
}

void Piece::stopAnimation()
{
    this->anim->stop();
}



void Piece::mousePressEvent(QGraphicsSceneMouseEvent *pe)
{
    if ( pe->button() == Qt::LeftButton && lpiece->inGame &&
         board->lboard->move == lpiece->isWhite && board->isPiecesSelectable()){
        this->select();
    }

    QGraphicsItem::mousePressEvent(pe);
}
